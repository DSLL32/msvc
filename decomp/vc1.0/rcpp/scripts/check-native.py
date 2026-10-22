#!/usr/bin/env python3
"""Native-only RCPP fixtures using immutable captured original byte oracles."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import copy
import signal

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
SCRATCH=Path.home()/'.ai-tmp/vc1.0-rcpp/native'

def sha(data): return hashlib.sha256(data).hexdigest()

def require_elf(path):
    data=path.read_bytes()[:20]
    assert data[:6]==b'\x7fELF\x01\x01'and data[18:20]==b'\x03\0','native fixture requires i386 ELF'

def checked(directory,record):
    data=(directory/record['path']).read_bytes()
    if 'bytes' in record: assert len(data)==record['bytes'],record['path']
    assert sha(data)==record['sha256'],record['path']
    return data

def load_spec(name):
    if name=='option-helper':
        return dict(fixture=None,arguments=[],exit=0,
                    stdout=b'10 examples and stdcall ABI match\r\n',stderr=b'',outputs={},extras={})
    config=json.loads((BINARY/'functions.json').read_text())
    setting=config['cases'][name]
    fixture=(BINARY/setting['fixture']).resolve()
    manifest=json.loads((fixture/'manifest.json').read_text())
    if name=='resources-rich':
        phase=manifest['phases']['rcpp']
        for record in manifest['toolchain_dependencies']: checked(ROOT,record)
        validation=json.loads((fixture/'validation.json').read_text())['phases']['rcpp']
        assert validation['stdout_bytes']==validation['stderr_bytes']==0
        assert (fixture/phase['stdout']).read_bytes()==(fixture/phase['stderr']).read_bytes()==b''
        for record in manifest['source_files']: checked(fixture,record)
        for record in phase['additional_inputs']: checked(fixture,record)
        outputs={phase['output']['name']:checked(fixture,phase['output'])}
        return dict(fixture=fixture,arguments=phase['arguments'],exit=phase['expected_exit'],
                    stdout=(fixture/phase['stdout']).read_bytes(),stderr=(fixture/phase['stderr']).read_bytes(),
                    outputs=outputs,extras={r['name']:checked(fixture,r)for r in phase['additional_inputs']})
    assert manifest['target_sha256']==config['target']['sha256']
    for record in manifest['source_inputs']: checked(fixture,record)
    for record in manifest['toolchain_dependencies']: checked(ROOT,record)
    case=next(c for c in manifest['cases']if c['id']==setting.get('manifest_case',name))
    return dict(fixture=fixture,arguments=[a.replace('{bin}',r'T:\bin').replace('{include}',r'T:\include')for a in case['arguments']],
                exit=case['expected_exit'],stdout=checked(fixture,case['stdout']),stderr=checked(fixture,case['stderr']),
                outputs={r['output_name']:checked(fixture,r)for r in case['outputs']},extras={})

def trace_module():
    spec=importlib.util.spec_from_file_location('rcpp_native_traces',Path(__file__).with_name('native-traces.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module

def deny_spawn(request):
    raise AssertionError('unexpected RCPP external dependency: '+request['file'])

def spawn_broker():
    spec=importlib.util.spec_from_file_location('rcpp_native_spawn',ROOT/'lib/recon/spawn.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module.SpawnBroker(deny_spawn)

def validate(work,spec,inputs,result):
    assert result.returncode==spec['exit'],('status',result.returncode,spec['exit'])
    assert result.stdout==spec['stdout'],('stdout',result.stdout,spec['stdout'])
    assert result.stderr==spec['stderr'],('stderr',result.stderr,spec['stderr'])
    for path,data in inputs.items(): assert (work/path).read_bytes()==data,('modified input',path)
    generated={p.relative_to(work).as_posix()for p in work.rglob('*')if p.is_file()}-set(inputs)
    assert generated==set(spec['outputs']),('output set',generated,set(spec['outputs']))
    for name,data in spec['outputs'].items(): assert (work/name).read_bytes()==data,('output bytes',name,len((work/name).read_bytes()),len(data))

def interrupt_run(command,work,env,extra,directory):
    """Deliver a real native signal only after an actual empty FIFO read."""
    pipes=directory/'console-interrupt.pipes';pipes.mkdir()
    fifo=pipes/'vc10-rcpp-interrupt';os.mkfifo(fifo)
    server=os.open(fifo,os.O_RDWR|os.O_NONBLOCK)
    env['RECON_NAMED_PIPE_ROOT']=str(pipes)
    process=subprocess.Popen(command,cwd=work,env=env,stdin=subprocess.DEVNULL,
                             stdout=subprocess.PIPE,stderr=subprocess.PIPE,**extra)
    proof={}
    try:
        deadline=time.monotonic()+10
        while time.monotonic()<deadline:
            assert process.poll()is None,'target exited before blocked FIFO boundary'
            proc=Path('/proc')/str(process.pid)
            try:
                syscall=(proc/'syscall').read_text().split()
                wait=(proc/'wchan').read_text().strip()
                fd=int(syscall[1],16)if syscall and syscall[0]=='3'else None
                if fd is not None and (proc/'fd'/str(fd)).resolve()==fifo and wait=='pipe_read':
                    assert (work/'result.i').read_bytes()==b'','output already flushed before signal'
                    proof=dict(pid=process.pid,blocked_syscall='read',read_fd=fd,wchan=wait,
                               fifo=str(fifo),before_output_bytes=0,signal='SIGINT',
                               signal_api='os.kill',windows_callback_thread_state_supported=False)
                    break
            except (FileNotFoundError,ProcessLookupError,ValueError):pass
            time.sleep(0.005)
        assert proof,'no verified blocked FIFO read'
        os.kill(process.pid,signal.SIGINT)
        out,err=process.communicate(timeout=10)
        proof.update(exit=process.returncode,after_output_bytes=len((work/'result.i').read_bytes()),
                     target_reaped=True,native_delivery='same-thread shared POSIX signal handler')
        (directory/'console-interrupt.signal.json').write_text(json.dumps(proof,indent=2)+'\n')
        return subprocess.CompletedProcess(command,process.returncode,out,err)
    finally:
        if process.poll()is None:process.kill();process.communicate()
        os.close(server)

def run_case(name,executable,directory,tracing=None):
    start=time.perf_counter();spec=load_spec(name)
    work=directory/name
    if spec['fixture']:shutil.copytree(spec['fixture']/'source',work)
    else:work.mkdir()
    for path,data in spec['extras'].items(): (work/path).write_bytes(data)
    inputs={p.relative_to(work).as_posix():p.read_bytes()for p in work.rglob('*')if p.is_file()}
    env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')}
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'):env.pop(key,None)
    env.update(RECON_DRIVE_T=str(ROOT/'toolchains/1.0-nt'),RECON_PROGRAM_PATH=r'T:\bin\RCPP.EXE',LC_ALL='C')
    extra={};trace_file=None;entry=None
    if tracing:
        entry=tracing['cache']['cases'][name]
        assert entry['terminal']==dict(kind='exit',code=spec['exit']), 'cached terminal mismatch'
        seen=tracing['cache']['accepted_application'][:entry['seen_count']]
        seen_path=directory/(name+'.seen');seen_path.write_text(''.join(n+'\n'for n in seen))
        trace_file=(directory/(name+'.trace')).open('wb')
        env.update(RECON_TRACE_FD=str(trace_file.fileno()),RECON_TRACE_FILTER=str(tracing['filter']),RECON_TRACE_SEEN=str(seen_path))
        extra['pass_fds']=(trace_file.fileno(),)
    broker=spawn_broker();env.update(broker.environment())
    extra['pass_fds']=extra.get('pass_fds',())+(broker.fileno(),)
    setup=time.perf_counter()-start;launch=time.perf_counter()
    try:
        program=BINARY/'build/native-trace/option-check' if name=='option-helper' else executable
        require_elf(program)
        if name=='console-interrupt':
            result=interrupt_run([str(program),*spec['arguments']],work,env,extra,directory)
        else:
            result=subprocess.run([str(program),*spec['arguments']],cwd=work,env=env,stdin=subprocess.DEVNULL,
                                  stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30,**extra)
    finally:
        if trace_file:trace_file.close()
        broker.close()
    broker.check()
    assert not broker.requests,'RCPP fixture unexpectedly requested a child'
    elapsed=time.perf_counter()-launch;check=time.perf_counter()
    (directory/(name+'.stdout')).write_bytes(result.stdout);(directory/(name+'.stderr')).write_bytes(result.stderr)
    validate(work,spec,inputs,result)
    if tracing:
        actual=(directory/(name+'.trace')).read_text().splitlines()
        assert actual==entry['expected'],('entry trace',name,actual,entry['expected'])
    record=dict(case=name,status='pass',exit=result.returncode,setup_seconds=setup,execution_seconds=elapsed,
                check_seconds=time.perf_counter()-check,total_seconds=time.perf_counter()-start,
                stdout_sha256=sha(result.stdout),stderr_sha256=sha(result.stderr),
                outputs={n:dict(bytes=len(d),sha256=sha(d))for n,d in spec['outputs'].items()},
                application_entries=len(entry['expected'])if entry else None)
    record['spawn_requests']=0
    record['process_boundary']='deny all external dependencies'
    if name=='console-interrupt':
        record['native_signal_proof']=json.loads((directory/'console-interrupt.signal.json').read_text())
        record['unsupported_original_state']=['Windows OS-created callback thread','retained Windows ReadFile return barrier','Windows exclusive file-sharing semantics']
    return record

def main():
    preparation_start=time.perf_counter()
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--trace',action='store_true');parser.add_argument('--case',action='append')
    parser.add_argument('--reject-tests',action='store_true')
    options=parser.parse_args();config=json.loads((BINARY/'functions.json').read_text())
    cases=options.case or list(config['cases'])+['option-helper']
    assert all(n in config['cases']or n=='option-helper'for n in cases),'unknown case'
    SCRATCH.mkdir(parents=True,exist_ok=True);directory=Path(tempfile.mkdtemp(prefix='trace-'if options.trace else'output-',dir=SCRATCH))
    executable=BINARY/('build/native-trace/RCPP.EXE'if options.trace else'build/native/RCPP.EXE')
    tracing=None
    if options.trace:
        cache=trace_module().load(BINARY/'build/native-trace')
        path=directory/'filter';path.write_text(''.join(n+'\n'for n in sorted(cache['application_definitions'])))
        tracing=dict(cache=cache,filter=path)
    if options.reject_tests:
        assert tracing, '--reject-tests requires --trace'
        rejected=[]
        fake=directory/'reject.exe';fake.write_bytes(b'MZ'+bytes(62))
        try:require_elf(fake)
        except AssertionError:rejected.append(dict(test='PE/non-ELF executable rejected before launch'))
        else:raise AssertionError('non-ELF accepted')
        probe=ROOT/'lib/recon/build/process-target';require_elf(probe)
        broker=spawn_broker();env={k:v for k,v in os.environ.items()if not k.startswith('RECON_')}
        env.update(broker.environment())
        try:
            result=subprocess.run([str(probe)],cwd=directory,env=env,pass_fds=(broker.fileno(),),
                                  stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
        finally:broker.close()
        assert result.returncode==125,('unknown dependency launched',result.returncode)
        try:broker.check()
        except AssertionError as error:
            assert ('unexpected RCPP external dependency: '+str(probe))in str(error),str(error)
            rejected.append(dict(test='actual native ELF self-spawn denied before child execution',status=result.returncode))
        else:raise AssertionError('unknown dependency broker accepted spawn')
        bad=copy.deepcopy(tracing['cache'])
        bad['cases']['long-suffix']['expected']=['rc_unsigned_kind']
        try:run_case('long-suffix',executable,directory,dict(cache=bad,filter=tracing['filter']))
        except AssertionError as error:
            assert error.args[0][0]=='entry trace',str(error)
            rejected.append(dict(test='wrong cached original first entry',reason=str(error)))
        else:raise AssertionError('wrong first entry accepted')
        # The first rejection left real successful native artifacts. Verify an
        # otherwise-valid oracle with one wrong output byte is rejected too.
        spec=load_spec('long-suffix');work=directory/'long-suffix'
        inputs={p.relative_to(work).as_posix():p.read_bytes()for p in work.rglob('*')if p.is_file()and p.name!='result.i'}
        spec['outputs']['result.i']+=b'x'
        result=subprocess.CompletedProcess([],0,spec['stdout'],spec['stderr'])
        try:validate(work,spec,inputs,result)
        except AssertionError as error:
            assert error.args[0][0]=='output bytes',str(error)
            rejected.append(dict(test='wrong captured artifact byte',reason=str(error)))
        else:raise AssertionError('wrong artifact accepted')
        reference=dict(path='long-suffix.rc',sha256='0'*64)
        try:checked(work,reference)
        except AssertionError:rejected.append(dict(test='wrong complete input digest'))
        else:raise AssertionError('unknown input accepted')
        report=dict(status='passed',rejections=rejected,capture_performed=False)
        (BINARY/'cases/native-rejection-validation.json').write_text(json.dumps(report,indent=2)+'\n')
        print('Native entry, artifact and input rejection checks passed.')
        return
    preparation_seconds=time.perf_counter()-preparation_start
    start=time.perf_counter();results=[]
    for name in cases:
        result=run_case(name,executable,directory,tracing);results.append(result);print(name+': native exact pass',flush=True)
    report=dict(status='passed',scope='Native-only captured status/streams/artifacts and projected application first entries; no Wine or recapture',
                executable_sha256=sha(executable.read_bytes()),capture_seconds=0,capture_performed=False,
                results=results,elapsed_seconds=time.perf_counter()-start,run_directory=str(directory),
                unsupported_state_observations=['Windows console callback-thread and read-return barrier state'])
    report['preparation_seconds']=preparation_seconds
    report['timing_units']='elapsed wall seconds; one sample, other workers active'
    report['timing_totals']={key:sum(r[key]for r in results)for key in ('setup_seconds','execution_seconds','check_seconds')}
    if tracing: report['trace_cache_sha256']=sha((BINARY/'cases/native-traces.json').read_bytes())
    (directory/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    (BINARY/'cases'/('native-trace-validation.json'if options.trace else'native-validation.json')).write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
