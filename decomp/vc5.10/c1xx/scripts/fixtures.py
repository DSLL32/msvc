#!/usr/bin/env python3
"""Native-only lifecycle replay against the explicitly frozen original oracle."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

PROJECT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('completion',Path(__file__).with_name('check-completion.py'))
d=importlib.util.module_from_spec(spec)
spec.loader.exec_module(d)


def main():
    started=time.perf_counter()
    result=subprocess.run(['make','-j4','native'],cwd=PROJECT,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    build=time.perf_counter()-started
    if result.returncode:
        raise RuntimeError(result.stdout.decode()+result.stderr.decode())
    reference=json.loads((PROJECT/'cases/lifecycle/reference.json').read_text())
    setup_started=time.perf_counter()
    scratch=Path.home()/'.ai-tmp/msvc2/vc5.10/c1xx'
    with tempfile.TemporaryDirectory(prefix='native-fixture-',dir=scratch) as temporary:
        output=Path(temporary)
        environment=os.environ.copy()
        for key in ('C1XX_INVOKE_ARGV','C1XX_TEST_INIT_RESTORE','C1XX_CRT_ARGV','C1XX_CRT_ENVIRONMENT',
                    'C1XX_SYSTEM_INFO_PROFILE','C1XX_SYSTEM_INFO_REFERENCE_SHA256','C1XX_SYSTEM_INFO_ORIGINAL'):
            environment.pop(key,None)
        # Start each case with its own known empty entry prefix.
        for key in ('RECON_TRACE_FD','RECON_TRACE_FILTER','RECON_TRACE_SEEN','RECON_TRACE_STATE_FD'):
            environment.pop(key,None)
        (output/'filter.txt').write_text('DllMain\n')
        (output/'seen.txt').write_text('')
        environment.update(RECON_TRACE_FILTER=str(output/'filter.txt'),RECON_TRACE_SEEN=str(output/'seen.txt'),
                           RECON_LIFECYCLE_STATE=str(output/'state.jsonl'))
        setup=time.perf_counter()-setup_started
        with (output/'trace.txt').open('wb') as trace:
            environment['RECON_TRACE_FD']=str(trace.fileno())
            run_started=time.perf_counter()
            run=subprocess.run([str(PROJECT/'build/native/c1xx-host')],cwd=PROJECT,env=environment,
                               pass_fds=(trace.fileno(),),stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
            execution=time.perf_counter()-run_started
        checks_started=time.perf_counter()
        d.s.require(run.returncode==reference['exit'] and run.stdout.hex()==reference['stdout_hex'] and
                    run.stderr.hex()==reference['stderr_hex'],'native status/streams differ from cached original')
        records=[json.loads(line) for line in (output/'state.jsonl').read_text().splitlines()]
        d.s.require(records[0]['registration']==reference['registration'],'native module provenance changed')
        d.s.require(d.native_states(records)==reference['typed_state'],'native lifecycle state differs')
        d.s.require((output/'trace.txt').read_text()==reference['trace'],'native application trace differs')
        # Inspect the compiled real Win32 DLL without running it or an external
        # dependency. Its exported functions retain their decorated stdcall ABI.
        from inventory import inventory
        image=inventory(PROJECT/'build/c1n32_l.dll')
        d.s.require(sorted((row['name'],row['ordinal']) for row in image['exports'])==
                    [('_AbortCompilerPass@4',1),('_InvokeCompilerPass@12',2)] and
                    image['characteristics']&0x2000,'rebuilt DLL export metadata differs')
        checks=time.perf_counter()-checks_started
    print(json.dumps({'case':reference['case'],'result':'PASS','wine_invocations':0,
                      'seconds':{'build':build,'setup':setup,'execution':execution,'checks':checks,
                                 'total':time.perf_counter()-started}}))


if __name__=='__main__':main()
