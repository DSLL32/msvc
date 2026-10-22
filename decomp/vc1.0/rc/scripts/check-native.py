#!/usr/bin/env python3
"""Native RC fixtures: frozen originals, application traces and strict RCPP replay."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
CONFIG=json.loads((BINARY/'functions.json').read_text())

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
processes=load('rc_native_processes',Path(__file__).with_name('native-processes.py'))
traces=load('rc_native_traces',Path(__file__).with_name('native-traces.py'))
def sha(data):return hashlib.sha256(data).hexdigest()
def case_paths(name):
    setting=CONFIG['cases'][name]
    if setting.get('fixture'):
        fixture=BINARY/setting['fixture'];oracle=fixture/setting['oracle']
    elif name=='resources-rich':fixture=BINARY.parent/'fixtures/resources-rich';oracle=fixture/'rc'
    else:fixture=BINARY/'cases'/name;oracle=fixture/'oracle'
    return fixture,oracle

def verify_inputs(name,fixture):
    manifest=json.loads((fixture/'manifest.json').read_text())
    setting=CONFIG['cases'][name]
    if setting.get('suite_manifest_sha256'):
        assert sha((fixture/'manifest.json').read_bytes())==setting['suite_manifest_sha256'],'fixture manifest pin differs'
    for item in manifest.get('source_inputs',[]):processes.reference(fixture,item)
    for path,item in manifest.get('inputs',{}).items():
        data=(fixture/'source'/path).read_bytes();assert len(data)==item['bytes'] and sha(data)==item['sha256'],'case input differs: '+path
    for item in manifest.get('toolchain_dependencies',[]):
        assert sha((ROOT/item['path']).read_bytes())==item['sha256'],'toolchain dependency changed: '+item['path']

def expected_files(name,fixture,oracle):
    files={str(p.relative_to(fixture/'source')):p.read_bytes() for p in (fixture/'source').rglob('*') if p.is_file()}
    if CONFIG['cases'][name].get('fixture'):
        manifest=json.loads((fixture/'manifest.json').read_text());case=next(c for c in manifest['cases'] if c['id']==name)
        for record in (case['stdout'],case['stderr'],*case['outputs']):processes.reference(fixture,record)
        for record in case['outputs']:files[record.get('output_name',Path(record['path']).name)]=(fixture/record['path']).read_bytes()
        streams={key:(fixture/case[key]['path']).read_bytes() for key in ('stdout','stderr')}
    else:
        streams={key:(oracle/(key+'.bin')).read_bytes() for key in ('stdout','stderr')}
        files.update({p.name:p.read_bytes() for p in oracle.iterdir() if p.is_file() and p.name not in ('stdout.bin','stderr.bin')})
    return files,streams

def run_console(executable,args,work,record,environment,pass_fds):
    script=record/'console.gdb';before=record/'before'
    target_out=record/'stdout';target_err=record/'stderr'
    lines=['set pagination off','set confirm off','set disable-randomization off','handle SIGINT nostop noprint pass',
           'break interrupt_resource','commands','silent','printf "RECON_CONTROL=%lu\\n", control','continue','end',
           'break initialize_lexer','run '+shlex.join(args)+' >'+shlex.quote(str(target_out))+' 2>'+shlex.quote(str(target_err)),
           'python import shutil; shutil.copytree('+repr(str(work))+','+repr(str(before))+')',
           'signal SIGINT','printf "RECON_EXIT=%d\\n", $_exitcode','quit']
    script.write_text('\n'.join(lines)+'\n')
    result=subprocess.run(['gdb','-q','-batch','-x',str(script),str(executable)],cwd=work,env=environment,
                          pass_fds=pass_fds,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
    (record/'gdb.stdout').write_bytes(result.stdout);(record/'gdb.stderr').write_bytes(result.stderr)
    assert result.returncode==0 and b'RECON_CONTROL=0\n' in result.stdout,(result.stdout,result.stderr)
    status=re.search(rb'RECON_EXIT=(\d+)\n',result.stdout);assert status,(result.stdout,result.stderr)
    return int(status[1]),target_out.read_bytes(),target_err.read_bytes()

def run_case(name,executable,directory,tracing):
    start=time.perf_counter();record=directory/name;record.mkdir();work=record/'work';work.mkdir()
    fixture,oracle=case_paths(name);verify_inputs(name,fixture)
    if (fixture/'source').exists():shutil.copytree(fixture/'source',work,dirs_exist_ok=True)
    setting=CONFIG['cases'][name];direct=setting.get('kind')=='direct-function-ABI'
    args=['--direct-'+setting['arguments']] if direct else shlex.split(setting['arguments'])
    files,streams=expected_files(name,fixture,oracle)
    env={k:v for k,v in os.environ.items() if not k.startswith('RECON_')}
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','WINEPREFIX','WINEARCH','WINEDEBUG'):env.pop(key,None)
    env.update(INCLUDE='T:\\include',WINEPATH='T:\\bin',RECON_DRIVE_T=str(ROOT/'toolchains/1.0-nt'),LC_ALL='C',
               RECON_PROGRAM_PATH='Z:\\root\\msvc2\\toolchains\\1.0-nt\\bin\\RC.EXE',TEMP=str(work),TMP=str(work))
    boundary=None;fds=[];entry=None
    if (BINARY/'cases/native-boundaries'/name/'manifest.json').exists():
        metadata=json.loads((BINARY/'cases/native-boundaries'/name/'manifest.json').read_text())
        env.update(metadata['environment']);env['RECON_PROGRAM_PATH']=metadata['program']
        if metadata.get('PATH'):
            env['PATH']=metadata['PATH'];env['RECON_SPAWN_REQUIRE_EXISTS']='1'
            # Preserve captured DOS identity without relying on the capture's
            # private launch tree. The only declared child is an inert PE input
            # whose response is replayed after strict identity/input checks.
            env['RECON_DRIVE_Z']=str(record/'dos-z')
            child=processes.host_path(metadata['application'],work,env)
            child.parent.mkdir(parents=True)
            child.symlink_to(ROOT/'toolchains/1.0-nt/bin/RCPP.EXE')
            program=processes.host_path(metadata['program'],work,env)
            program.parent.mkdir(parents=True)
            assert not (program.parent/'rcpp.exe').exists()
        boundary=processes.ProcessFixture(name,work,env);fds.append(boundary.fileno())
    trace=None;state=None
    if tracing:
        entry=tracing['cache']['cases'][name]
        seen=record/'seen';seen.write_text(''.join(n+'\n' for n in entry['seen']))
        trace=(record/'trace.entries').open('wb');state=(record/'trace.state').open('w+b')
        state.truncate(32+len(tracing['cache']['application_definitions']))
        env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(tracing['filter']),
                   RECON_TRACE_SEEN=str(seen),RECON_TRACE_STATE_FD=str(state.fileno()))
        fds += [trace.fileno(),state.fileno()]
    setup=time.perf_counter()-start;execution=time.perf_counter()
    try:
        if name=='console-interrupt':status,stdout,stderr=run_console(executable,args,work,record,env,tuple(fds))
        else:
            result=subprocess.run([str(executable),*args],cwd=work,env=env,pass_fds=tuple(fds),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
            status,stdout,stderr=result.returncode,result.stdout,result.stderr
        execution=time.perf_counter()-execution
    finally:
        if boundary:boundary.close()
        if trace:trace.close();state.close()
    (record/'stdout').write_bytes(stdout);(record/'stderr').write_bytes(stderr)
    check_start=time.perf_counter()
    if boundary:
        (record/'spawn.json').write_text(json.dumps(boundary.receipt(),indent=2)+'\n');boundary.check()
    assert status==setting.get('exit',0),(name,'exit',status)
    if name=='verbose-names':
        assert boundary and len(boundary.calls)==1
        digits=('%06d'%boundary.calls[0]['pid'])[-6:].encode()
        expected=bytearray(streams['stdout']);expected[161:167]=digits;expected[173:179]=digits
        assert len(stdout)==349 and stdout==expected,'verbose stdout differs beyond the two validated native PID fields'
    else:assert stdout==streams['stdout'],(name,'stdout',stdout,streams['stdout'])
    assert stderr==streams['stderr'],(name,'stderr',stderr,streams['stderr'])
    observed={str(p.relative_to(work)):p.read_bytes() for p in work.rglob('*') if p.is_file()}
    assert observed==files,(name,'complete artifact/input set differs',[(n,len(d)) for n,d in observed.items()],[(n,len(d)) for n,d in files.items()])
    if name=='console-interrupt':
        before={p.name:p.read_bytes() for p in (record/'before').iterdir() if p.is_file()}
        call=boundary.calls[0];metadata=boundary.metadata;response=bytearray(boundary.response)
        for offset in metadata['temporary_input_response_offsets']:response[offset:offset+8]=call['input'].encode()
        assert before=={'interrupt.rc':files['interrupt.rc'],'interrupt.res':b'',call['input']:boundary.input,call['output']:bytes(response)}
    if tracing:
        raw=(record/'trace.entries').read_bytes();actual=raw.decode('ascii').splitlines()
        assert actual==entry['expected'],(name,'application first-entry trace differs',actual,entry['expected'])
    return dict(case=name,status='pass',exit=status,setup_seconds=setup,execution_seconds=execution,
                checks_seconds=time.perf_counter()-check_start,total_seconds=time.perf_counter()-start,
                stdout_bytes=len(stdout),stderr_bytes=len(stderr),artifacts={n:dict(bytes=len(d),sha256=sha(d)) for n,d in observed.items()},
                trace_entries=len(entry['expected']) if entry else None,
                process_fixture=boundary.receipt() if boundary else None,
                scope=('Actual native SIGINT delivered at GDB pre-lexer boundary; exact cleanup. Windows console membership and OS callback thread remain original-only evidence.' if name=='console-interrupt' else
                       'Direct native utility results/canaries; canonical stdcall/nonvolatile ABI audit retained separately.' if direct else 'Native CLI with captured original RCPP process response when required'))

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--case',action='append',choices=CONFIG['cases'])
    ap.add_argument('--trace',action='store_true');ap.add_argument('--directory',type=Path);ap.add_argument('--trace-cache',type=Path,default=traces.CACHE)
    options=ap.parse_args();executable=BINARY/('build/native-trace/RC.EXE' if options.trace else 'build/native/RC.EXE')
    assert executable.read_bytes()[:4]==b'\x7fELF'
    directory=options.directory or Path(tempfile.mkdtemp(prefix='native-check-',dir=Path.home()/'.ai-tmp/vc1.0-rc'))
    directory.mkdir(exist_ok=True);directory=directory.resolve();setup=time.perf_counter();tracing=None
    if options.trace:
        cache=traces.load(BINARY/'build/native-trace',options.trace_cache)
        filter_path=directory/'filter';filter_path.write_text(''.join(n+'\n' for n in cache['application_definitions']))
        tracing=dict(cache=cache,filter=filter_path)
    receipt=dict(mode='native-only captured-reference',build_seconds='separate build; not measured by fixture runner',
                 trace_setup_seconds=time.perf_counter()-setup,cases=[],executable_sha256=sha(executable.read_bytes()),Wine_invocations=0)
    start=time.perf_counter()
    for name in options.case or CONFIG['cases']:
        begun=time.perf_counter()
        try:result=run_case(name,executable,directory,tracing)
        except Exception as error:result=dict(case=name,status='fail',error=str(error),total_seconds=time.perf_counter()-begun)
        receipt['cases'].append(result);print(name,result['status'],'%.4fs'%result['total_seconds'],result.get('error',''),flush=True)
    receipt.update(total_seconds=time.perf_counter()-start,status='pass' if all(c['status']=='pass' for c in receipt['cases']) else 'fail')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('receipt:',directory/'receipt.json')
    return 0 if receipt['status']=='pass' else 1
if __name__=='__main__':raise SystemExit(main())
