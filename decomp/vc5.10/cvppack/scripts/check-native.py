#!/usr/bin/env python3
"""Native-only regression of the current explicitly frozen help frontier/result."""
import importlib.util
import argparse
import json
import os
from pathlib import Path
import resource
import subprocess
import sys
import tempfile
import time
from state import BINARY, ROOT, load, sha


def environment(reference):
    env={key:value for key,value in os.environ.items() if not key.startswith(('WINE','RECON_','VC510_CVPACK_'))}
    env.update(LC_ALL='C',TZ='UTC',PATH='',
        RECON_DRIVE_R=str(ROOT),RECON_DRIVE_S=str(Path.home()/'.ai-tmp/msvc2'),
        RECON_PROGRAM_PATH=reference['actual_target_windows'],VC510_CVPACK_ARGV0=reference['canonical_argv0'])
    return env


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case',choices=['help','hello-debug-dll'],default='help')
    args=parser.parse_args()
    start=time.perf_counter()
    config,_=load()
    case=json.loads((BINARY/'cases'/args.case/'case.json').read_text())
    from caseio import definition,prepare
    inputs=definition(args.case)
    reference_dir=BINARY/case['reference']
    reference=json.loads((reference_dir/'receipt.json').read_text())
    assert sha(reference_dir/'receipt.json')==case['reference_receipt_sha256'], 'changed frozen reference'
    assert reference['original_sha256']==config['target']['sha256']
    for item in reference['outputs']:
        assert sha(reference_dir/item['name'])==item['sha256'], 'changed original evidence'
    spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
    decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
    with tempfile.TemporaryDirectory(prefix='cv510-cvpack-fixture-',dir=Path.home()/'.ai-tmp') as temporary:
        output=Path(temporary)
        prepare(inputs,output)
        preparation=time.perf_counter()
        resource_spec=importlib.util.spec_from_file_location('native_resources',BINARY/'scripts/native-resources.py')
        resources=importlib.util.module_from_spec(resource_spec);resource_spec.loader.exec_module(resources)
        resource_provider=resources.prepare(output/'resources.rmr')
        widen_provider=None
        if args.case=='hello-debug-dll' and case['terminal']=='exit':
            spec=importlib.util.spec_from_file_location('native_widen',BINARY/'scripts/native-widen.py')
            widen=importlib.util.module_from_spec(spec);spec.loader.exec_module(widen)
            widen_provider=widen.prepare(output/'widen.rwp')
        preparation_seconds=time.perf_counter()-preparation
        if case['terminal']=='trap':
            capture=output/'native'
            execution_start=time.perf_counter()
            result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(reference_dir),
                                   '--output',str(capture)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
            assert result.returncode==0,(result.stdout,result.stderr)
            elapsed=time.perf_counter()-execution_start
            state=json.loads((capture/'state.json').read_text())
            widen_provider=json.loads((capture/'receipt.json').read_text()).get('prepared_widen')
            decoder.compare(config,json.loads((BINARY/'data.json').read_text()),(reference_dir/'events').read_text(),state,
                {name:(reference_dir/('artifact-'+name)).read_bytes() for name in inputs['artifacts']})
        else:
            assert case['terminal']=='exit'
            capture=output
            filter_path=output/'filter'
            filter_path.write_text(''.join(r['native_symbol']+'\n' for r in config['functions']))
            seen=output/'seen';seen.write_bytes(b'')
            env=environment(reference)
            env['VC510_CVPACK_RESOURCES']=str(output/'resources.rmr')
            if widen_provider:env['VC510_CVPACK_WIDEN']=str(output/'widen.rwp')
            def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
            execution_start=time.perf_counter()
            with (output/'entry-trace').open('wb') as trace,(output/'stdout').open('wb') as stdout,(output/'stderr').open('wb') as stderr:
                env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(filter_path),RECON_TRACE_SEEN=str(seen))
                child=subprocess.run([str(BINARY/'build/native/CVPACK'),*case['argv']],cwd=output,env=env,stdin=subprocess.DEVNULL,
                    stdout=stdout,stderr=stderr,pass_fds=(trace.fileno(),),timeout=10,preexec_fn=no_core)
            elapsed=time.perf_counter()-execution_start
            assert child.returncode==case['exit_status']
        checking=time.perf_counter()
        assert (capture/'entry-trace').read_text().splitlines()==case['application_trace']
        for name in ('stdout','stderr'):
            assert (capture/name).read_bytes()==(reference_dir/name).read_bytes(), name+' mismatch'
        for name in inputs['artifacts']:
            artifact=capture/('artifact-'+name) if case['terminal']=='trap' else output/name
            assert artifact.read_bytes()==(reference_dir/('artifact-'+name)).read_bytes(),'artifact mismatch'
        report=dict(schema=1,case=args.case,terminal=case['terminal'],native_only=True,
            native_sha256=sha(BINARY/'build/native/CVPACK'),reference_receipt_sha256=case['reference_receipt_sha256'],
            prepared_resources=resource_provider,
            prepared_widen=widen_provider,
            seconds=dict(preparation=preparation_seconds,execution=elapsed,checks=time.perf_counter()-checking,total=time.perf_counter()-start))
        (BINARY/('build/native-fixture'+('' if args.case=='help' else '-'+args.case)+'.json')).write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(report,indent=2))


if __name__=='__main__':main()
