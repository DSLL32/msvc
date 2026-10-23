#!/usr/bin/env python3
"""Cached Linux-only LINK child regression; no debugger, Wine or PE execution."""
import argparse
import contextlib
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from build_inputs import verify_native_build
from case_inputs import load,copy_inputs,copy_dependencies,dependencies
from link_provider import Provider,verify

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
digest=lambda raw:hashlib.sha256(raw).hexdigest()
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--config',type=Path);args=parser.parse_args()
started=time.perf_counter();reference=PROJECT/'cases/link-object/reference';master=json.loads((reference/'fixture.json').read_bytes())
case=load('link-object');config=json.loads((args.config or PROJECT/'functions.json').read_bytes())
assert master['inputs']==case['inputs'] and master['arguments']==case['arguments']
assert set(master['required_accepted'])<=set(config['accepted'])
build=time.perf_counter();environment=os.environ.copy()
for key in ('MAKEFLAGS','MFLAGS'):environment.pop(key,None)
subprocess.run(['make','--no-print-directory','-j4','native-trace'],cwd=PROJECT,env=environment,check=True,stdout=subprocess.DEVNULL)
build=time.perf_counter()-build;binary=PROJECT/'build/native-trace/CL.EXE';binary_hash=digest(binary.read_bytes())
receipt=json.loads(Path(str(binary)+'.build.json').read_bytes());verify_native_build(receipt,binary_hash,PROJECT)
def shared(name,directory):
    raw=subprocess.check_output(['git','show',receipt['shared_revision']+':lib/recon/'+name+'.py'],cwd=ROOT)
    path=directory/(name+'_reader.py');path.write_bytes(raw);spec=importlib.util.spec_from_file_location('link_fixture_'+name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module,path
with tempfile.TemporaryDirectory(prefix='fixture-link-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary);setup=time.perf_counter();(directory/'config.json').write_text(json.dumps(config,indent=2)+'\n');copy_inputs(case,directory);copy_dependencies(dependencies('link-object'),directory)
    shutil.copyfile(reference/'resources.bin',directory/'resources.bin')
    assert digest((directory/'resources.bin').read_bytes())==master['native_resource_provider_sha256']
    expected_trace=(reference/'trace.txt').read_bytes();assert digest(expected_trace)==master['trace_sha256']
    (directory/'trace-filter.txt').write_text(''.join(row['native_symbol']+'\n'for row in config['driver']))
    (directory/'trace-seen.txt').write_text('')
    scratch=Path('/root/.ai-tmp/msvc2');historical=Path(master['program_path'][3:].replace('\\','/')).parent
    mirror=directory/'drive-s'/historical;mirror.parent.mkdir(parents=True);mirror.symlink_to(directory,target_is_directory=True)
    status_module,status_path=shared('status',directory);mktemp_module,mktemp_path=shared('mktemp',directory);spawn_module,spawn_path=shared('spawn',directory)
    with contextlib.ExitStack()as contexts:
        terminal=contexts.enter_context(status_module.TerminalStatus());call=master['temporary_name_boundary']['calls'][0]
        observation=contexts.enter_context(mktemp_module.MktempObservation(call['template'],call['result'][-6:],[call['before'][name]for name in ('errno','doserrno','last_error')]))
        provider=Provider(reference/'original',directory,spawn_path,cached_fixture=True)
        try:
            for key,value in master['environment'].items():
                if value is None:environment.pop(key,None)
                else:environment[key]=value
            environment.update(provider.environment(),**terminal.environment(),**observation.environment(),
                CL_ENTRY_PROFILE=master['entry_profile'],RECON_PROGRAM_PATH=master['program_path'],RECON_DRIVE_S=str(directory/'drive-s'),RECON_DRIVE_R=str(ROOT),
                RECON_RESOURCE_PROVIDER=str(directory/'resources.bin'),RECON_TRACE_FILTER=str(directory/'trace-filter.txt'),RECON_TRACE_SEEN=str(directory/'trace-seen.txt'))
            with (directory/'stdout.bin').open('wb')as out,(directory/'stderr.bin').open('wb')as err,(directory/'trace.txt').open('wb')as trace:
                environment['RECON_TRACE_FD']=str(trace.fileno());provider.arm();setup=time.perf_counter()-setup
                execution=time.perf_counter();process=subprocess.Popen(master['argv'],executable=str(binary),cwd=directory,env=environment,
                    stdin=subprocess.DEVNULL,stdout=out,stderr=err,pass_fds=(*provider.descriptors(),terminal.fd,observation.fd,trace.fileno()))
                process.wait(timeout=10);execution=time.perf_counter()-execution
            checks=time.perf_counter();(directory/'native-process.json').write_text(json.dumps({'pid':process.pid})+'\n')
            status=terminal.read(process.pid,process.returncode);assert status==0
            observed=observation.read(process.pid)
            assert observed['before']==call['template'] and observed['after']==call['result'] and observed['argument']==observed['result']
            for phase in ('before','after'):
                assert {name:observed[phase+'_'+name]for name in ('errno','doserrno','last_error')}==call[phase]
            provider.finish(process.pid);verify(directory,cached_fixture=True)
            try:verify(directory,cached_fixture=False)
            except AssertionError:pass
            else:raise AssertionError('cached LINK fixture was accepted as fresh proof')
            for name in ('stdout','stderr'):
                assert digest((reference/(name+'.bin')).read_bytes())==master['streams'][name]
                assert (directory/(name+'.bin')).read_bytes()==(reference/(name+'.bin')).read_bytes()
            assert (directory/'trace.txt').read_bytes()==expected_trace
            checks=time.perf_counter()-checks
        finally:provider.close()
report={'passed':True,'case':'link-object','mode':'cached native regression; fresh GDB proof separate','application_entries':len(expected_trace.splitlines()),'status':status,
    'object_input_bytes':275,'response_input_bytes':82,'executable_output_bytes':1024,'LINK_calls':1,'mktemp_calls':1,'native_sha256':binary_hash,
    'seconds':{'build':build,'setup':setup,'execution':execution,'checks':checks,'total':time.perf_counter()-started}}
(PROJECT/'build/fixture-link-times.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
