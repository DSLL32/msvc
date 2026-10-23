#!/usr/bin/env python3
"""Explicit original-only RCPP/RC capture in fixed R:/S:; never a fixture step."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from case_paths import capture_lock, windows

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
wt=load('rc_reference_wine',Path(__file__).with_name('wine-tool.py'))
checker=load('rc_reference_oracles',Path(__file__).with_name('check-native.py'))

def sha(data):return hashlib.sha256(data).hexdigest()
def record(path,base):
    data=path.read_bytes();return dict(path=os.path.relpath(path,base),bytes=len(data),sha256=sha(data))
def source_directory(name,case):return checker.case_paths(name)[0]/'source'
def files(directory):return {str(p.relative_to(directory)):p.read_bytes() for p in directory.rglob('*') if p.is_file()}

def capture(name,case,destination,base,cache,config):
    total=time.perf_counter();destination.mkdir()
    work=base/'work';raw=base/'raw'
    for p in (work,raw):
        if p.exists():shutil.rmtree(p)
    shutil.copytree(source_directory(name,case),work);raw.mkdir(parents=True)
    env=wt.environment();target=wt.TC/'bin/RC.EXE';path=None
    if name=='path-fallback':
        launch=base/'launch';children=base/'preprocessor bin'
        launch.mkdir(exist_ok=True);children.mkdir(exist_ok=True)
        target=launch/'rc.exe';shutil.copyfile(wt.TC/'bin/RC.EXE',target)
        child=children/'rcpp.exe'
        if not child.is_symlink():child.symlink_to(wt.TC/'bin/RCPP.EXE')
        assert child.resolve()==wt.TC/'bin/RCPP.EXE'
        path=';;'+windows(base/'no-such-directory')+';;"'+windows(children)+'";;'
        env.update(WINEPATH='',RC_REFERENCE_PATH=path)
        assert not (launch/'rcpp.exe').exists()
    plan=base/'entries.txt'
    drivers={r['name']:r for r in config['driver']}
    plan.write_text(''.join('%x %s\n'%(drivers[r['logical']]['start']-config['target']['image_base'],n)
                            for n,r in cache['application_definitions'].items()))
    env['RC_REFERENCE_ENTRIES']=windows(plan)
    setup=time.perf_counter()-total
    command=['/usr/bin/wine',windows(BINARY/'build/capture-boundary.exe'),windows(target),
             'ebc0','12680',windows(raw),windows(work),case['arguments']]
    start=time.perf_counter()
    result=subprocess.run(command,env=env,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=120)
    capture_seconds=time.perf_counter()-start
    for stream,data in [('debugger.stdout',result.stdout),('debugger.stderr',result.stderr)]:
        (raw/stream).write_bytes(data);shutil.copyfile(raw/stream,destination/stream)
    expected=0 if name=='console-interrupt' else case.get('exit',0)
    assert result.returncode==expected,(name,result.returncode,result.stdout,result.stderr)
    pid=int(re.search(rb'original pid (\d+)\r?\n',result.stdout)[1])
    entries=re.findall(rb'entry (\w+)\r?\n',result.stdout)
    entries=[n.decode() for n in entries]
    projected=[n for n in entries if n not in cache['cases'][name]['seen']]
    # The real console callback is captured separately; this run only observes
    # its preprocessor boundary and normal main-thread path.
    expected_entries=[] if name=='console-interrupt' else cache['cases'][name]['expected']
    assert projected==expected_entries,(name,'fresh original entry order differs',projected,expected_entries)
    fixture,oracle=checker.case_paths(name);expected_files,streams=checker.expected_files(name,fixture,oracle)
    actual=files(work)
    main_stdout=(raw/'stdout.bin').read_bytes();main_stderr=(raw/'stderr.bin').read_bytes()
    if name!='console-interrupt':
        assert actual==expected_files,(name,'fresh original complete products/source set differs')
        expected_stdout=bytearray(streams['stdout'])
        if name=='verbose-names':
            digits=('%06d'%pid)[-6:].encode();expected_stdout[161:167]=digits;expected_stdout[173:179]=digits
        assert main_stdout==expected_stdout and main_stderr==streams['stderr'],(name,'fresh original streams differ')
    for kind,data in [('RC.stdout.bin',main_stdout),('RC.stderr.bin',main_stderr)]:
        (destination/kind).write_bytes(data)
    products=[]
    initial=files(source_directory(name,case))
    for relative,data in actual.items():
        if relative in initial:assert data==initial[relative];continue
        out=destination/'products'/relative;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(data)
        products.append(record(out,destination))
    manifest=dict(schema=2,case=name,kind='captured-original-RC-case',arguments=case['arguments'],
        target_sha256=sha((wt.TC/'bin/RC.EXE').read_bytes()),program=windows(target),
        canonical_cwd=windows(work),original_RC_pid=pid,original_RC_exit=result.returncode,
        original_RC_stdout=record(destination/'RC.stdout.bin',destination),original_RC_stderr=record(destination/'RC.stderr.bin',destination),
        original_RC_products=products,complete_file_set={n:dict(bytes=len(d),sha256=sha(d)) for n,d in actual.items()},
        original_application_entries=entries,seeded_application_entries=projected,
        application_projection=dict(seen=cache['cases'][name]['seen'],expected=expected_entries),
        console_signal='not generated; preprocessor boundary only; real signal has a separate explicit original proof' if name=='console-interrupt' else None,
        debugger_stdout=record(destination/'debugger.stdout',destination),debugger_stderr=record(destination/'debugger.stderr',destination),
        capture_namespace=dict(prefix=str(wt.PREFIX),R=str(ROOT),S=str(wt.SCRATCH)),
        environment={key:env[key] for key in ('INCLUDE','WINEPATH')},PATH=path,
        static_argv_provenance=dict(array_va='0x0101b018',first_pointer='0x0101b008',options_pointer='0x01018670',
            argv0='RCPP.EXE',spawnv_call_va='0x01007acb',spawnvp_call_va='0x01007af0'))
    child_exits=re.findall(rb'child exit (\d+) pid (\d+)',result.stdout)
    repeat_seconds=0
    if name not in ('help','invalid-switch','missing-input'):
        assert len(child_exits)==1,(name,'child status missing',result.stdout)
        child_status=int(child_exits[0][0]);assert child_status==(1 if name=='preprocessor-error' else 0)
        application=(raw/'application.txt').read_bytes().decode('latin1')
        line=(raw/'command.txt').read_bytes().decode('latin1')
        match=re.fullmatch(r'RCPP.EXE  -f (RC\d{6}) -g (RD\d{6})(.*)',line);assert match,(name,line)
        input_name,output_name=match.group(1,2)
        assert input_name=='RC'+('%06d'%pid)[-6:] and output_name=='RD'+('%06d'%pid)[-6:]
        deleted={p.read_bytes().decode('latin1'):p.with_suffix('.bin') for p in raw.glob('deleted-*.name')}
        assert input_name in deleted and output_name in deleted
        prepared=deleted[input_name].read_bytes();response=deleted[output_name].read_bytes()
        (destination/'input.bin').write_bytes(prepared);(destination/'response.bin').write_bytes(response)
        shutil.copyfile(raw/'command.txt',destination/'command.txt')
        # Repeat at the SAME canonical cwd, so absolute include paths remain
        # evidence rather than a normalizable transport detail.
        shutil.rmtree(work);shutil.copytree(source_directory(name,case),work)
        repeat_out=base/'independent-output'
        if repeat_out.exists():shutil.rmtree(repeat_out)
        repeat_out.mkdir();(work/input_name).write_bytes(prepared)
        cpp_command=['/usr/bin/wine',windows(BINARY/'build/capture-boundary.exe'),application,'0','0',
                     windows(repeat_out),windows(work),windows(destination/'command.txt'),'replay']
        env.pop('RC_REFERENCE_ENTRIES');start=time.perf_counter()
        again=subprocess.run(cpp_command,env=env,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=120)
        repeat_seconds=time.perf_counter()-start
        assert again.returncode==child_status and (work/output_name).read_bytes()==response,(name,'independent original RCPP differs',again.stdout,again.stderr)
        stdout=(repeat_out/'stdout.bin').read_bytes();stderr=(repeat_out/'stderr.bin').read_bytes();assert not stdout
        for stream,data in [('stdout.bin',stdout),('stderr.bin',stderr),('independent.stdout',again.stdout),('independent.stderr',again.stderr)]:
            (destination/stream).write_bytes(data)
        reads=[]
        for file in sorted(raw.glob('read-*.name')):
            path_text=file.read_bytes().decode('latin1');data=file.with_suffix('.bin')
            item=dict(path=path_text,exists=data.exists())
            if data.exists():
                out=destination/('read-'+str(len(reads))+'.bin');shutil.copyfile(data,out);item['capture']=record(out,destination)
            reads.append(item)
        assert reads and any(r['path'].upper().endswith(input_name) for r in reads)
        offsets=[m.start() for m in re.finditer(re.escape(input_name.encode()),response)]
        for offset in offsets:assert response[max(0,offset-16):offset].endswith(b'"') and response[offset+8:offset+9]==b'"'
        manifest.update(kind='captured-original-RCPP-process-boundary',child_sha256=sha((wt.TC/'bin/RCPP.EXE').read_bytes()),
            application=application,command_line=line,argv=['RCPP.EXE',line[len('RCPP.EXE '):]],
            input_name=input_name,output_name=output_name,input=record(destination/'input.bin',destination),response=record(destination/'response.bin',destination),
            stdout=record(destination/'stdout.bin',destination),stderr=record(destination/'stderr.bin',destination),exit=child_status,reads=reads,
            temporary_input_response_offsets=offsets,independently_verified=True,
            independent_stdout=record(destination/'independent.stdout',destination),independent_stderr=record(destination/'independent.stderr',destination))
    else:assert not child_exits
    manifest['timing']=dict(setup_seconds=setup,original_capture_seconds=capture_seconds,independent_original_seconds=repeat_seconds,total_seconds=time.perf_counter()-total)
    (destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(name,'original capture+entries verified',len(projected),'entries',flush=True)
    return dict(case=name,manifest_sha256=sha((destination/'manifest.json').read_bytes()),**manifest['timing'])

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--case',action='append')
    ap.add_argument('--directory',type=Path,required=True);args=ap.parse_args()
    config=json.loads((BINARY/'functions.json').read_text());cache=json.loads((BINARY/'cases/native-traces.json').read_text())
    names=[n for n,c in config['cases'].items() if c.get('kind')!='direct-function-ABI'];selected=args.case or names
    assert all(n in names for n in selected);args.directory=args.directory.resolve();args.directory.mkdir(exist_ok=True)
    start=time.perf_counter();receipts=[]
    for name in selected:
        destination=args.directory/name;assert not destination.exists(),'Refusing to overwrite original capture: '+name
        with capture_lock(name) as base:receipts.append(capture(name,config['cases'][name],destination,base,cache,config))
    (args.directory/'capture-validation.json').write_text(json.dumps(dict(status='pass',cases=receipts,seconds=time.perf_counter()-start,
        build_seconds='separate explicit original-toolchain helper build'),indent=2)+'\n')
if __name__=='__main__':main()
