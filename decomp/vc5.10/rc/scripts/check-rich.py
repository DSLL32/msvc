#!/usr/bin/env python3
"""Native-only rich RC fixture against the shared original dependency capture."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from common import HERE,ROOT,read,sha,symbol,verify_compiled_sources
from rich_reference import CASE,REFERENCE,ARGUMENTS,verify


def verify_case_tree(directory,source_inputs):
    files={str(path.relative_to(directory)) for path in directory.rglob('*') if path.is_file()}
    assert files==set(source_inputs),'Unknown/missing dependency input file'
    for name,pin in source_inputs.items():
        assert sha(directory/name)==pin['sha256'],'Dependency input bytes differ '+name


def check(record=None):
    started=time.perf_counter()
    settings=read(HERE/'functions.json')
    assert {row['name'] for row in settings['driver']}<=set(settings['accepted'])
    specification,provider,headers,observed=verify()
    compiled=read(HERE/'build/native/rc.inputs.json');verify_compiled_sources(compiled)
    native_pin=sha(HERE/'build/native/rc');compiled_pin=sha(HERE/'build/native/rc.inputs.json')
    with tempfile.TemporaryDirectory(prefix='vc510-rc-rich-',dir=str(Path.home()/'.ai-tmp')) as temporary:
        temporary=Path(temporary);directory=temporary/'case';controls=temporary/'controls'
        directory.mkdir();controls.mkdir()
        for name in provider['source_inputs']:
            destination=directory/name;destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(CASE/'source'/name,destination)
        verify_case_tree(directory,provider['source_inputs'])
        (controls/'filter').write_text(''.join(symbol(row['name'])+'\n' for row in settings['driver']))
        (controls/'seen').write_text('')
        environment={key:value for key,value in os.environ.items() if not key.startswith(('RECON_','WINE','RC_'))}
        environment.update(RC_DEPENDENCY_RESPONSE=str(REFERENCE/'rcdll-response.bin'),RC_DEPENDENCY_LOG=str(controls/'boundary.log'),RC_DEPENDENCY_CWD=observed['cwd'],RECON_PROGRAM_PATH=observed['before'][0]['text'])
        setup=time.perf_counter()-started
        with (controls/'trace').open('wb') as trace:
            environment.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(controls/'filter'),RECON_TRACE_SEEN=str(controls/'seen'))
            execution=time.perf_counter()
            result=subprocess.run([str(HERE/'build/native/rc')]+ARGUMENTS,cwd=directory,env=environment,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,pass_fds=(trace.fileno(),),timeout=10)
            execution=time.perf_counter()-execution
        verification=time.perf_counter()
        assert result.returncode==0,(result.returncode,result.stderr)
        for stream,actual in [('stdout.bin',result.stdout),('stderr.bin',result.stderr)]:
            assert actual==(REFERENCE/stream).read_bytes(),'Rich stream differs '+stream
        expected_trace=[symbol(name) for name in observed['trace']]
        assert (controls/'trace').read_text().splitlines()==expected_trace,'Driver trace differs from actual original main/callback events'
        expected_log='RC 0 0 6\n'
        for name,pin in sorted(provider['source_inputs'].items()):expected_log+='input %s %u\n'%(name,pin['bytes'])
        for index,row in enumerate(observed['after']):expected_log+='argv-after %u %u %u\n'%(index,row['original_allocation'],row['offset'])
        expected_log+='artifact resources.res 3672\nreturn 0\n'
        assert (controls/'boundary.log').read_text()==expected_log,'Complete dependency input/alias/artifact sequence differs'
        expected_files=set(provider['source_inputs'])|{'resources.res'}
        actual_files={str(path.relative_to(directory)) for path in directory.rglob('*') if path.is_file()}
        assert actual_files==expected_files,'Unknown/missing rich artifact'
        for name,pin in provider['source_inputs'].items():
            assert sha(directory/name)==pin['sha256'],'Target modified input '+name
        assert (directory/'resources.res').read_bytes()==(REFERENCE/'resources.res').read_bytes(),'Resource artifact differs'
        verification=time.perf_counter()-verification
        assert sha(HERE/'build/native/rc')==native_pin and sha(HERE/'build/native/rc.inputs.json')==compiled_pin,'Native artifact changed during fixture'
        if record is not None:
            record=Path(record);assert record.is_relative_to(Path.home()/'.ai-tmp'),'Keep execution artifacts in scratch'
            record.mkdir(parents=True,exist_ok=False)
            for source,name in [(controls/'trace','trace.txt'),(controls/'boundary.log','boundary.txt'),(directory/'resources.res','resources.res'),(HERE/'build/native/rc.inputs.json','compiled-sources.json'),(HERE/'build/native/rc','native.elf')]:shutil.copy2(source,record/name)
            (record/'stdout.bin').write_bytes(result.stdout);(record/'stderr.bin').write_bytes(result.stderr)
    report={'status':'pass','case':'resources-rich','status_code':result.returncode,'stdout_bytes':len(result.stdout),'stderr_bytes':len(result.stderr),'resource_bytes':3672,'resource_sha256':sha(REFERENCE/'resources.res'),'case_inputs':len(provider['source_inputs']),'conservative_headers':len(headers['files']),'post_argv_aliases':[(row['original_allocation'],row['offset']) for row in observed['after']],'native_trace':expected_trace,'setup_seconds':setup,'execution_seconds':execution,'verification_seconds':verification,'native_sha256':native_pin,'native_compiled_receipt_sha256':compiled_pin,'case_manifest_sha256':sha(REFERENCE/'manifest.json'),'target_functions_added':0,'DLL_coverage_claimed':False,'wine_invocations':0}
    if record is not None:
        report['files']={path.name:{'bytes':path.stat().st_size,'sha256':sha(path)} for path in record.iterdir() if path.is_file()}
        (record/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--record',type=Path);args=parser.parse_args();print(json.dumps(check(args.record),indent=2))
