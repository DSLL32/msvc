#!/usr/bin/env python3
"""Native-only authored driver regression against cached streams, trace and API effects."""
import contextlib
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

PROJECT=Path(__file__).resolve().parents[1]
ROOT=PROJECT.parents[2]
sha=lambda data:hashlib.sha256(data).hexdigest()


def shared(name,revision):
    cache=Path('/root/.ai-tmp/msvc2/vc5.10/cl/shared-source-checkouts')/revision/'lib/recon'/(name+'.py')
    assert cache.read_bytes()==subprocess.check_output(['git','show',revision+':lib/recon/'+name+'.py'],cwd=ROOT)
    spec=importlib.util.spec_from_file_location('fixture_'+name,cache)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module


parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('case');parser.add_argument('--config',type=Path)
args=parser.parse_args()
from case_inputs import load,copy_inputs
case=load(args.case)
started=time.perf_counter();reference=PROJECT/'cases'/args.case/'reference'
master=json.loads((reference/'fixture.json').read_text());config=json.loads((args.config or PROJECT/'functions.json').read_text())
assert master['target_sha256']==config['target']['sha256']
assert set(master['required_accepted'])<=set(config['accepted']),'help prerequisite is unaccepted'
build_environment=os.environ.copy()
for key in ('MAKEFLAGS','MFLAGS'):build_environment.pop(key,None)
build=time.perf_counter()
subprocess.run(['make','--no-print-directory','-j4','native-trace'],cwd=PROJECT,env=build_environment,check=True,stdout=subprocess.DEVNULL)
build=time.perf_counter()-build
receipt=json.loads((PROJECT/'build/native-trace/CL.EXE.build.json').read_text())
status_reader=shared('status',receipt['shared_revision'])
temporary_reader=shared('mktemp',receipt['shared_revision'])
scratch=Path('/root/.ai-tmp/msvc2/vc5.10/cl');scratch.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='fixture-driver-',dir=scratch) as work:
    directory=Path(work);setup=time.perf_counter()
    assert master['inputs']==case['inputs']and master['arguments']==case['arguments']
    copy_inputs(case,directory)
    shutil.copyfile(reference/'resources.bin',directory/'resources.bin')
    assert sha((directory/'resources.bin').read_bytes())==master['native_resource_provider_sha256']
    expected_trace=(reference/'trace.txt').read_bytes();assert sha(expected_trace)==master['trace_sha256']
    # Include every application entry, so newly reached functions cannot hide
    # behind an old accepted-only trace filter.
    (directory/'filter').write_text(''.join(r['native_symbol']+'\n'for r in config['driver']))
    (directory/'seen').write_text('')
    environment=os.environ.copy()
    for key,value in master['environment'].items():
        if value is None:environment.pop(key,None)
        else:environment[key]=value
    environment.update(CL_ENTRY_PROFILE=master['entry_profile'],RECON_PROGRAM_PATH=master['program_path'],
        RECON_RESOURCE_PROVIDER=str(directory/'resources.bin'),RECON_TRACE_FILTER=str(directory/'filter'),RECON_TRACE_SEEN=str(directory/'seen'))
    call=master['temporary_name_boundary']['calls'][0]if master['temporary_name_boundary']is not None else None
    with contextlib.ExitStack()as contexts:
        terminal=contexts.enter_context(status_reader.TerminalStatus())
        environment.update(terminal.environment());temporary=None
        if call is not None:
            prestate=[call['before'][key]for key in ('errno','doserrno','last_error')]
            temporary=contexts.enter_context(temporary_reader.MktempObservation(call['template'],call['result'][-6:],prestate))
            environment.update(temporary.environment())
        else:
            # An unbound receipt makes every unexpected _mktemp call deny125;
            # this captured zero-call case supplies no manufactured response.
            for key in ('RECON_MKTEMP_TEMPLATE','RECON_MKTEMP_SUFFIX','RECON_MKTEMP_PRESTATE'):
                environment.pop(key,None)
            environment['RECON_MKTEMP_FD']='-1'
        with (directory/'trace').open('wb')as trace,(directory/'stdout').open('wb')as out,(directory/'stderr').open('wb')as err:
            environment['RECON_TRACE_FD']=str(trace.fileno());setup=time.perf_counter()-setup
            execution=time.perf_counter()
            process=subprocess.Popen(master['argv'],executable=str(PROJECT/'build/native-trace/CL.EXE'),cwd=directory,
                env=environment,stdin=subprocess.DEVNULL,stdout=out,stderr=err,
                pass_fds=(trace.fileno(),terminal.fd)+((temporary.fd,)if temporary is not None else()))
            process.wait(timeout=10);execution=time.perf_counter()-execution
        checks=time.perf_counter()
        status=terminal.read(process.pid,process.returncode)
        assert status==master['reference_status']==0,'full DWORD status differs'
        observation=None
        if temporary is not None:
            observation=temporary.read(process.pid)
            assert observation['before']==call['template']and observation['after']==call['result']
            assert observation['argument']==observation['result']and call['result_alias']
            for phase in ('before','after'):
                assert {key:observation[phase+'_'+key]for key in ('errno','doserrno','last_error')}==call[phase]
    for label in ('stdout','stderr'):
        expected=(reference/(label+'.bin')).read_bytes()
        assert sha(expected)==master['streams'][label]
        assert (directory/label).read_bytes()==expected,label+' differs'
    assert (directory/'trace').read_bytes()==expected_trace,'application first-entry trace differs'
    assert {p.name for p in directory.iterdir()}=={'resources.bin','filter','seen','trace','stdout','stderr'}|{item['name']for item in case['inputs']},'unexpected output artifact'
    checks=time.perf_counter()-checks
report={'schema':1,'case':args.case,'passed':True,'full_dword_status':status,
    'application_entries':len(expected_trace.splitlines()),'stdout_bytes':(reference/'stdout.bin').stat().st_size,
    'stderr_bytes':(reference/'stderr.bin').stat().st_size,'mktemp_calls':observation['count']if observation is not None else 0,
    'native_sha256':sha((PROJECT/'build/native-trace/CL.EXE').read_bytes()),
    'seconds':{'build':build,'setup':setup,'execution':execution,'checks':checks,'total':time.perf_counter()-started}}
(PROJECT/('build/fixture-'+args.case+'-times.json')).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
