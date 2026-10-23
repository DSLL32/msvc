#!/usr/bin/env python3
"""Cached help replay: execute only the instrumented i386 Linux reconstruction."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from environment_profile import prepare

BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def application_symbols(executable,configuration):
    """Resolve actual ELF names, including the two genuine member declarations."""
    output=subprocess.run(['nm','--defined-only',str(executable)],capture_output=True,check=True).stdout.decode()
    names=[line.split()[-1] for line in output.splitlines() if len(line.split())==3 and line.split()[1] in ('t','T')]
    demangled=subprocess.run(['c++filt'],input=('\n'.join(names)+'\n').encode(),capture_output=True,check=True).stdout.decode().splitlines()
    assert len(names)==len(demangled)
    result={}
    for record in configuration['driver']:
        logical=record['name']
        native=record.get('native_symbol','link_main' if logical=='_main' else logical.lstrip('_'))
        matches=[name for name,readable in zip(names,demangled)
                 if name==native or ('::' in native and readable.startswith(native+'('))]
        assert len(matches)==1, 'missing/ambiguous ELF symbol for '+logical
        assert matches[0] not in result, 'ambiguous application ELF name'
        result[matches[0]]=logical
    return result


def load_reference(configuration):
    master_path=BINARY/'cases/help/completed/master.json'
    assert master_path.is_file(), 'completed help reference is missing'
    master=json.loads(master_path.read_text())
    assert master['schema']==1 and master['case_arguments']==configuration['cases']['help']['arguments']==['/?']
    assert master['target_sha256']==configuration['target']['sha256']
    assert master['status']==1100 and master['posix_status']==76
    reference=master_path.parent/'original'
    receipt_bytes=(reference/'receipt.json').read_bytes()
    assert sha(receipt_bytes)==master['receipt_sha256'], 'completed reference receipt changed'
    receipt=json.loads(receipt_bytes)
    assert receipt['mode']=='masked' and receipt['helper_status']==0
    assert receipt['actual_loaded_target_hfile_verified'] and receipt['actual_loaded_crt_verified']
    assert receipt['original_sha256']==master['target_sha256']
    for name in ['events.jsonl','stdout.bin','stderr.bin']:
        data=(reference/name).read_bytes();record=receipt['files'][name]
        assert len(data)==record['bytes'] and sha(data)==record['sha256'], 'cached original changed: '+name
    events=[json.loads(line) for line in (reference/'events.jsonl').read_text().splitlines()]
    assert not [event for event in events if event['event']=='trap'], 'completion reference trapped'
    assert events[-1]==dict(event='summary',failed=0,status=1100)
    assert [event for event in events if event['event']=='exit']==[dict(event='exit',status=1100)]
    applications={record['name'] for record in configuration['driver']}
    trace=[event['name'] for event in events if event['event']=='entry' and event['name'] in applications]
    assert trace==master['application_trace'] and len(trace)==len(set(trace))
    assert set(trace)<=set(configuration['accepted'])
    for name in ['stdout.bin','stderr.bin']:
        data=(reference/name).read_bytes()
        assert len(data)==master['streams'][name]['bytes'] and sha(data)==master['streams'][name]['sha256']
    # The original unmodified run supplies an independent stream/status master.
    baseline=BINARY/'cases/help/original'
    provenance=json.loads((baseline/'provenance.json').read_text())
    assert provenance['status']==1100
    for name in ['stdout.bin','stderr.bin']:
        assert (reference/name).read_bytes()==(baseline/name).read_bytes()
    return reference,master


def main():
    started=time.perf_counter()
    configuration=json.loads((BINARY/'functions.json').read_text())
    if not configuration.get('cases',{}).get('help',{}).get('complete'):
        raise SystemExit('Help is not accepted: named traps remain; no fixture success claimed.')
    subprocess.run(['make','-q','native'],cwd=BINARY,check=True)
    executable=BINARY/'build/native/LINK.EXE'
    assert executable.read_bytes()[:4]==b'\x7fELF', 'fixture requires the native ELF'
    reference,master=load_reference(configuration)
    scratch=Path.home()/'.ai-tmp/msvc2/vc5.10/link'
    work=Path(tempfile.mkdtemp(prefix='fixture-help-',dir=scratch))
    symbol_map=application_symbols(executable,configuration)
    function_filter=work/'application.txt'
    function_filter.write_text(''.join(name+'\n' for name in sorted(symbol_map)))
    seen=work/'seen.txt';seen.write_bytes(b'')
    trace=work/'trace.txt'
    profile,reference_sha=prepare(reference,work)
    environment={key:value for key,value in os.environ.items() if not key.startswith('RECON_')}
    for key in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB','LINK_DB','LINK_REPRO','LINK_REPRO_NAME']:
        environment.pop(key,None)
    environment.update(RECON_PROGRAM_PATH=r'R:\toolchains\5.10\bin\LINK.EXE',
                       RECON_SYSTEM_PROFILE=str(profile),RECON_SYSTEM_REFERENCE=reference_sha,
                       RECON_TRACE_FILTER=str(function_filter),RECON_TRACE_SEEN=str(seen))
    spec=importlib.util.spec_from_file_location('shared_terminal_status',ROOT/'lib/recon/status.py')
    shared=importlib.util.module_from_spec(spec);spec.loader.exec_module(shared)
    setup=time.perf_counter()-started
    with shared.TerminalStatus() as terminal,trace.open('wb') as output, \
            (work/'stdout.bin').open('wb') as stdout_file,(work/'stderr.bin').open('wb') as stderr_file:
        environment.update(terminal.environment(),RECON_TRACE_FD=str(output.fileno()))
        before=time.perf_counter()
        process=subprocess.Popen([str(executable),'/?'],cwd=work,env=environment,
                                 pass_fds=(terminal.fd,output.fileno()),stdout=stdout_file,stderr=stderr_file)
        try:process.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill();process.communicate();raise
        execution=time.perf_counter()-before
        assert process.returncode==master['posix_status'], 'wrong native POSIX status'
        logical_status=terminal.read(process.pid,process.returncode)
        assert logical_status==master['status'], 'wrong full logical DWORD status'
    before=time.perf_counter()
    stdout=(work/'stdout.bin').read_bytes();stderr=(work/'stderr.bin').read_bytes()
    assert stdout==(reference/'stdout.bin').read_bytes(), 'help stdout differs'
    assert stderr==(reference/'stderr.bin').read_bytes(), 'help stderr differs'
    actual=[symbol_map[name] for name in trace.read_text().splitlines()]
    assert actual==master['application_trace'], 'help application entry order differs'
    checks=time.perf_counter()-before
    report=dict(case='help',status='passed',logical_status=logical_status,posix_status=process.returncode,
        application_entries=len(actual),native_sha256=sha(executable.read_bytes()),
        reference_receipt_sha256=reference_sha,work_directory=str(work),
        elapsed_seconds=dict(setup=setup,execution=execution,checks=checks,total=time.perf_counter()-started),
        build='Makefile native prerequisite, outside these runner timings; no original/ref recapture.')
    (work/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    (BINARY/'build/help-fixture.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
