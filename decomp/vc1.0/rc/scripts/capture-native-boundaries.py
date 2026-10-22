#!/usr/bin/env python3
"""Explicit original-only RCPP reference capture; never imported by fixtures."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
spec=importlib.util.spec_from_file_location('rc_reference_wine',Path(__file__).with_name('wine-tool.py'))
wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)

def sha(data):return hashlib.sha256(data).hexdigest()
def record(path,base):
    data=path.read_bytes();return dict(path=os.path.relpath(path,base),bytes=len(data),sha256=sha(data))
def source_directory(name,case):
    if case.get('fixture'):return BINARY/case['fixture']/'source'
    if name=='resources-rich':return BINARY.parent/'fixtures/resources-rich/source'
    return BINARY/'cases'/name/'source'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',action='append')
    ap.add_argument('--directory',type=Path,default=BINARY/'cases/native-boundaries')
    args=ap.parse_args()
    config=json.loads((BINARY/'functions.json').read_text())
    names=[n for n,c in config['cases'].items() if c.get('kind')!='direct-function-ABI' and n not in ('help','invalid-switch','missing-input')]
    selected=args.case or names
    assert all(n in names for n in selected)
    args.directory.mkdir(exist_ok=True)
    scratch=Path.home()/'.ai-tmp/vc1.0-rc/native-boundary-capture';scratch.mkdir(exist_ok=True)
    capture_start=time.perf_counter();receipts=[]
    for name in selected:
        total=time.perf_counter();case=config['cases'][name]
        destination=args.directory/name
        assert not destination.exists(), 'frozen boundary already exists: '+name
        destination.mkdir()
        work=scratch/(name+'-work');raw=scratch/(name+'-raw')
        for p in (work,raw):
            if p.exists():shutil.rmtree(p)
        shutil.copytree(source_directory(name,case),work);raw.mkdir()
        env=wt.environment();target=wt.TC/'bin/RC.EXE'
        path=None
        if name=='path-fallback':
            launch=scratch/'launch';children=scratch/'preprocessor bin'
            launch.mkdir(exist_ok=True);children.mkdir(exist_ok=True)
            target=launch/'rc.exe';shutil.copyfile(wt.TC/'bin/RC.EXE',target)
            child=children/'rcpp.exe'
            if not child.is_symlink():child.symlink_to(wt.TC/'bin/RCPP.EXE')
            path=';;'+wt.windows(scratch/'no-such-directory')+';;"'+wt.windows(children)+'";;'
            env.update(WINEPATH='',RC_REFERENCE_PATH=path)
            assert not (launch/'rcpp.exe').exists()
        setup=time.perf_counter()-total
        command=['/usr/bin/wine',str(BINARY/'build/capture-boundary.exe'),wt.windows(target),
                 'ebc0','12680',wt.windows(raw),wt.windows(work),case['arguments']]
        start=time.perf_counter()
        result=subprocess.run(command,env=env,cwd=work,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=120)
        capture_seconds=time.perf_counter()-start
        (raw/'debugger.stdout').write_bytes(result.stdout);(raw/'debugger.stderr').write_bytes(result.stderr)
        expected=0 if name=='console-interrupt' else case.get('exit',0)
        assert result.returncode==expected,(name,result.returncode,result.stdout,result.stderr)
        child_exits=re.findall(rb'child exit (\d+) pid (\d+)',result.stdout)
        assert len(child_exits)==1,(name,'child status missing',result.stdout)
        child_status=int(child_exits[0][0]);assert child_status==(1 if name=='preprocessor-error' else 0)
        application=(raw/'application.txt').read_bytes().decode('latin1')
        line=(raw/'command.txt').read_bytes().decode('latin1')
        match=re.fullmatch(r'RCPP.EXE  -f (RC\d{6}) -g (RD\d{6})(.*)',line)
        assert match,(name,line)
        input_name,output_name=match.group(1,2)
        deleted={p.read_bytes().decode('latin1'):p.with_suffix('.bin') for p in raw.glob('deleted-*.name')}
        assert input_name in deleted and output_name in deleted,(name,deleted)
        prepared=deleted[input_name].read_bytes();response=deleted[output_name].read_bytes()
        (destination/'input.bin').write_bytes(prepared);(destination/'response.bin').write_bytes(response)
        shutil.copyfile(raw/'command.txt',destination/'command.txt')
        # Independently launch untouched original RCPP with the exact captured
        # command line, canonical tool identity, inherited environment and files.
        repeat=scratch/(name+'-independent');repeat_out=scratch/(name+'-independent-output')
        for p in (repeat,repeat_out):
            if p.exists():shutil.rmtree(p)
        shutil.copytree(source_directory(name,case),repeat);repeat_out.mkdir()
        (repeat/input_name).write_bytes(prepared)
        cpp_command=['/usr/bin/wine',str(BINARY/'build/capture-boundary.exe'),application,'0','0',
                     wt.windows(repeat_out),wt.windows(repeat),wt.windows(destination/'command.txt'),'replay']
        start=time.perf_counter()
        again=subprocess.run(cpp_command,env=env,cwd=repeat,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=120)
        repeat_seconds=time.perf_counter()-start
        assert again.returncode==child_status,(name,again.returncode,again.stdout,again.stderr)
        assert (repeat/output_name).read_bytes()==response,(name,'independent RCPP output differs')
        stdout=(repeat_out/'stdout.bin').read_bytes();stderr=(repeat_out/'stderr.bin').read_bytes()
        assert not stdout
        (destination/'stdout.bin').write_bytes(stdout);(destination/'stderr.bin').write_bytes(stderr)
        # Every attempted CreateFileA read, including missing search candidates.
        reads=[]
        for file in sorted(raw.glob('read-*.name')):
            path_text=file.read_bytes().decode('latin1');data=file.with_suffix('.bin')
            item=dict(path=path_text,exists=data.exists())
            if data.exists():
                out=destination/('read-'+str(len(reads))+'.bin');shutil.copyfile(data,out)
                item['capture']=record(out,destination)
            reads.append(item)
        assert reads and any(r['path'].upper().endswith(input_name) for r in reads)
        offsets=[m.start() for m in re.finditer(re.escape(input_name.encode()),response)]
        for offset in offsets:
            assert response[max(0,offset-16):offset].endswith(b'"') and response[offset+len(input_name):offset+len(input_name)+1]==b'"'
        manifest=dict(schema=1,case=name,kind='captured-original-RCPP-process-boundary',
            target_sha256=sha((wt.TC/'bin/RC.EXE').read_bytes()),child_sha256=sha((wt.TC/'bin/RCPP.EXE').read_bytes()),
            arguments=case['arguments'],program=wt.windows(target),application=application,command_line=line,argv=['RCPP.EXE',line[len('RCPP.EXE '):]],
            input_name=input_name,output_name=output_name,input=record(destination/'input.bin',destination),
            response=record(destination/'response.bin',destination),stdout=record(destination/'stdout.bin',destination),
            stderr=record(destination/'stderr.bin',destination),exit=child_status,reads=reads,
            temporary_input_response_offsets=offsets,environment=dict(INCLUDE='T:\\include',WINEPATH='' if path else 'T:\\bin'),
            PATH=path,cwd='source-only isolated case working directory',
            original_RC_exit=result.returncode,console_signal='not generated; this capture observes RCPP boundary only' if name=='console-interrupt' else None,
            independently_verified=True,timing=dict(setup_seconds=setup,original_capture_seconds=capture_seconds,
                 independent_original_seconds=repeat_seconds,total_seconds=time.perf_counter()-total),
            debugger_stdout=record(raw/'debugger.stdout',destination),debugger_stderr=record(raw/'debugger.stderr',destination))
        # Preserve debugger evidence alongside the frozen fixture.
        for stream in ('stdout','stderr'):
            shutil.copyfile(raw/('debugger.'+stream),destination/('debugger.'+stream))
            manifest['debugger_'+stream]=record(destination/('debugger.'+stream),destination)
        (destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        receipts.append(dict(case=name,**manifest['timing']))
        print(name,'captured+independent:',len(prepared),len(response),'bytes',flush=True)
    (args.directory/'capture-validation.json').write_text(json.dumps(dict(status='pass',cases=receipts,
        seconds=time.perf_counter()-capture_start,build_seconds='separate explicit original-toolchain helper build; not timed here'),indent=2)+'\n')

if __name__=='__main__':main()
