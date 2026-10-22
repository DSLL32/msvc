#!/usr/bin/env python3
"""Native i386 CL fixture and cached original application-entry regression.

No Wine, reference capture, PE execution, or automatic oracle refresh.
"""
import argparse
import importlib.machinery
import json
import os
from pathlib import Path
import re
import signal
import shlex
import subprocess
import tempfile
import time

HERE=Path(__file__).resolve().parent

def module(name):
    return importlib.machinery.SourceFileLoader('cl_'+name.replace('-','_'),str(HERE/(name+'.py'))).load_module()

cases=module('native-cases');traces=module('native-traces');processes=module('native-processes');artifacts=module('native-artifacts')
SCRATCH=Path.home()/'.ai-tmp/vc1.0-cl/native-check'


def execute(executable,case,environment,work,streams,pass_fds,receipt_directory):
    command=[str(executable),*windows_arguments(case['arguments'].format(**cases.substitutions(case['id'])))]
    mode=case.get('mode')
    if mode in ('omf-state','ide-null-buffer'):
        script=receipt_directory/'state.gdb'
        breakpoint='link_pass' if mode=='omf-state' else 'ide_message'
        mutation='set variable coff_format = 0' if mode=='omf-state' else 'set variable ide_callbacks->buffer = 0'
        script.write_text('set pagination off\nset confirm off\nset startup-with-shell on\nbreak '+breakpoint+'\ncommands\nsilent\n'+mutation+'\ndisable 1\ncontinue\nend\nrun\n')
        target_arguments=command[1:]
        command=['/usr/bin/gdb','--batch','--quiet','--return-child-result','-x',str(script),'--args',*command]
        # GDB stdout must not contaminate target streams. Inferior uses these
        # explicit descriptor paths, inherited by both debugger and inferior.
        script.write_text(script.read_text().replace('run\n','run '+shlex.join(target_arguments)+' > /proc/self/fd/%d 2> /proc/self/fd/%d\n'%(streams[0].fileno(),streams[1].fileno())))
        result=subprocess.run(command,cwd=work,env=environment,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,pass_fds=(*pass_fds,streams[0].fileno(),streams[1].fileno()),timeout=30)
        (receipt_directory/'gdb.stdout').write_bytes(result.stdout);(receipt_directory/'gdb.stderr').write_bytes(result.stderr)
        return result.returncode
    if mode=='handler-idle':
        with subprocess.Popen(command,cwd=work,env=environment,stdin=subprocess.PIPE,stdout=streams[0],stderr=streams[1],pass_fds=pass_fds) as child:
            deadline=time.monotonic()+10
            while b'(press <return> to continue)' not in (receipt_directory/'stdout.bin').read_bytes():
                if child.poll() is not None:raise AssertionError('idle interrupt exited before prompt')
                assert time.monotonic()<deadline, 'idle interrupt did not reach input wait'
                time.sleep(.005)
            child.send_signal(signal.SIGINT)
            return child.wait(timeout=10)
    return subprocess.run(command,cwd=work,env=environment,stdin=subprocess.DEVNULL,stdout=streams[0],stderr=streams[1],pass_fds=pass_fds,timeout=30).returncode


def windows_arguments(text):
    # The existing cases use the VC startup parser's double-quote grouping;
    # backslashes remain literal except immediately before a quote.
    result=[];token=[];quoted=False;index=0;active=False
    while index<len(text):
        char=text[index]
        if char=='"':quoted=not quoted;active=True
        elif char in ' \t' and not quoted:
            if active:result.append(''.join(token));token=[];active=False
        else:token.append(char);active=True
        index+=1
    assert not quoted, 'unbalanced case quotation'
    if active:result.append(''.join(token))
    return result


def verify_frozen_artifacts(reference,oracle,directory):
    if 'products' in oracle:
        expected={name:row['original'] for name,row in oracle['products'].items()}
        assert set(reference['final'])==set(expected), 'captured artifact names differ from frozen oracle'
    else:
        expected=oracle.get('artifacts',{})
        assert set(reference['final'])==set(reference['initial'])|set(expected), 'captured artifact names differ from historical case'
    for name,row in expected.items():
        data=processes.contents(reference['final'][name])
        assert len(data)==row['bytes'], 'captured artifact length differs from frozen oracle: '+name
        temporary=directory/('frozen-artifact'+Path(name).suffix)
        temporary.write_bytes(data)
        normalized,_=artifacts.normalized_artifact(temporary);temporary.unlink()
        assert cases.sha(normalized)==row['normalized_sha256'], 'captured artifact differs from frozen oracle: '+name


def run(case,cache,trace,root):
    begin=time.perf_counter();case_id=case['id'];directory=root/case_id;directory.mkdir()
    work=directory/case_id;cases.prepare(case,work)
    reference=json.loads((processes.STORE/(case_id+'.json')).read_text())
    oracle=cases.oracles()[case_id]
    verify_frozen_artifacts(reference,oracle,directory)
    assert processes.inventory(work)==reference['initial'], 'initial case input differs from original capture'
    environment={k:v for k,v in os.environ.items() if not k.startswith('RECON_') and k not in ('CL','_CL_','_MSC_IDE_FLAGS','LINK','_LINK_','MSC_CMD_FLAGS','TMP','TEMP','TZ')}
    environment.update(reference['environment'])
    environment.update(RECON_DRIVE_Q=str(work.parent),RECON_PROGRAM_PATH=cases.windows(cases.BINARY/'build/native-reference.exe'),LC_ALL='C',SHELL='/bin/sh',TZ=reference['environment'].get('TZ','PST8PDT'))
    # Pin the original temporary basename for byte-exact invocation and trees.
    for boundary in reference['boundaries']:
        for entry in boundary.get('environment') or []:
            match=re.search(r' -il (.*?) -f ',entry)
            if match:
                environment['RECON_MKTEMP_SUFFIX']=match[1][-6:];break
        if 'RECON_MKTEMP_SUFFIX' not in environment:
            for argument in boundary['argv']:
                match=re.fullmatch(r'@Q:\\[^\\]+\\([A-Za-z0-9]{6})lk',argument)
                if match:environment['RECON_MKTEMP_SUFFIX']=match[1];break
        if 'RECON_MKTEMP_SUFFIX' in environment:break
    mode=case.get('mode') or ('clock' if case_id=='timing' else None)
    if mode=='clock':
        year,month,day,hour=case.get('clock_calendar',[2026,1,15,0])
        environment['RECON_LOCAL_CLOCK']='%04d-%02d-%02dT%02d:00:00.900/1225'%(year,month,day,hour)
    filter_path=directory/'filter.txt';seen_path=directory/'seen.txt';trace_path=directory/'trace.txt'
    descriptors=[]
    if trace:
        segment=cache['cases'][case_id]
        filter_path.write_text(''.join(name+'\n' for name in sorted(cache['application_definitions'])))
        seen_path.write_text(''.join(name+'\n' for name in cache['accepted_application'][:segment['seen_count']]))
        fd=os.open(trace_path,os.O_WRONLY|os.O_CREAT|os.O_TRUNC,0o600);descriptors.append(fd)
        environment.update(RECON_TRACE_FD=str(fd),RECON_TRACE_FILTER=str(filter_path),RECON_TRACE_SEEN=str(seen_path))
        state_fd=os.open(directory/'seen-state.bin',os.O_RDWR|os.O_CREAT|os.O_TRUNC,0o600)
        os.ftruncate(state_fd,32+len(cache['application_definitions']));descriptors.append(state_fd)
        environment['RECON_TRACE_STATE_FD']=str(state_fd)
    process=processes.ProcessFixture(case,reference,work)
    environment.update(process.broker.environment());descriptors.append(process.broker.fileno())
    executable=cases.BINARY/'build'/('native-trace' if trace else 'native')/'CL.EXE'
    assert executable.read_bytes()[:5]==b'\x7fELF\x01', 'native executable must be i386 ELF'
    setup=time.perf_counter()-begin;begin=time.perf_counter()
    with (directory/'stdout.bin').open('wb') as stdout,(directory/'stderr.bin').open('wb') as stderr:
        try:status=execute(executable,case,environment,work,(stdout,stderr),tuple(descriptors),directory)
        finally:
            for fd in descriptors[:-1]:os.close(fd)
            process.finish()
    execution=time.perf_counter()-begin;begin=time.perf_counter()
    assert status==case['expected_exit'], 'status differs: '+str(status)
    for name in ('stdout','stderr'):
        actual=(directory/(name+'.bin')).read_bytes()
        expected=oracle['streams'][name];digest=expected if isinstance(expected,str) else expected['sha256']
        assert cases.sha(actual)==digest, name+' differs; actual='+repr(actual)
        assert actual==processes.contents(reference[name]), 'capture differs from frozen streams'
    expected_files=reference['final']
    actual_files={str(p.relative_to(work)):p for p in work.rglob('*') if p.is_file()}
    assert set(actual_files)==set(expected_files), 'final artifact set differs: '+repr((set(actual_files)-set(expected_files),set(expected_files)-set(actual_files)))
    for name,path in actual_files.items():
        expected=directory/'reference-artifact';expected.write_bytes(processes.contents(expected_files[name]))
        a,_=artifacts.normalized_artifact(path)
        # Preserve filename suffix for .map parsing.
        target=directory/('reference'+path.suffix);expected.rename(target)
        b,_=artifacts.normalized_artifact(target);target.unlink()
        assert a==b, 'artifact bytes differ outside identified timestamps: '+name
    oracle_checks=time.perf_counter()-begin;begin=time.perf_counter()
    emitted=trace_path.read_text().splitlines() if trace else []
    if trace:assert emitted==segment['expected'], 'application frontier sequence differs: '+repr((emitted,segment['expected']))
    trace_checks=time.perf_counter()-begin
    return dict(id=case_id,status='passed',exit_status=status,application_entries=emitted,children=process.broker.requests,
                timing=dict(setup_seconds=setup,execution_seconds=execution,oracle_checks_seconds=oracle_checks,trace_checks_seconds=trace_checks))


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--case',action='append');parser.add_argument('--trace',action='store_true');parser.add_argument('--output',type=Path)
    options=parser.parse_args();start=time.perf_counter();registry=cases.specifications()
    selected=options.case or list(registry)
    SCRATCH.mkdir(parents=True,exist_ok=True);root=options.output or Path(tempfile.mkdtemp(prefix='run-',dir=SCRATCH));root.mkdir(parents=True,exist_ok=True)
    prepare=time.perf_counter();processes.verify_toolchain()
    cache=traces.load(cases.BINARY/'build/native-trace') if options.trace else None
    preparation=time.perf_counter()-prepare;rows=[];status='passed'
    for case_id in selected:
        try:
            row=run(registry[case_id],cache,options.trace,root);rows.append(row);print(case_id,'passed',row['timing'],flush=True)
        except Exception as error:
            rows.append(dict(id=case_id,status='failed',error=str(error)));status='failed';print(case_id,'FAILED',error,flush=True)
    receipt=dict(status=status,trace=options.trace,dependency_and_trace_setup_seconds=preparation,total_seconds=time.perf_counter()-start,cases=rows)
    (root/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('receipt:',root/'receipt.json')
    if status!='passed':raise SystemExit(1)

if __name__=='__main__':main()
