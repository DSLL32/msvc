#!/usr/bin/env python3
"""Frozen-reference native-only first-case fixture. Never imports original helpers."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from common import HERE,ROOT,read,sha,symbol
from reference_data import verify as verify_reference


def check():
    settings=read(HERE/'functions.json')
    assert {r['name'] for r in settings['driver']}<=set(settings['accepted']),'Application functions remain unaccepted'
    verify_reference(settings)
    reference=HERE/'cases/help/reference'
    manifest=read(reference/'manifest.json')
    for name,pin in manifest['files'].items():
        path=reference/name;assert path.stat().st_size==pin['bytes'] and sha(path)==pin['sha256'],'Reference changed '+name
    for name,pin in manifest['dependencies'].items():assert sha(ROOT/name)==pin,'Dependency changed '+name
    start=time.perf_counter()
    with tempfile.TemporaryDirectory(prefix='vc510-rc-native-',dir=str(Path.home()/'.ai-tmp')) as temp:
        directory=Path(temp)
        (directory/'filter').write_text(''.join(symbol(r['name'])+'\n' for r in settings['driver']))
        (directory/'seen').write_text('')
        env={k:v for k,v in os.environ.items() if not k.startswith(('RECON_','WINE'))}
        env.update(RC_DEPENDENCY_RESPONSE=str(reference/'rcdll-response.bin'),RC_DEPENDENCY_LOG=str(directory/'boundary.log'),RECON_PROGRAM_PATH='S:\\vc5.10\\rc\\original\\RC.EXE')
        setup=time.perf_counter()-start
        with (directory/'trace').open('wb') as trace:
            env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(directory/'filter'),RECON_TRACE_SEEN=str(directory/'seen'))
            execute=time.perf_counter()
            result=subprocess.run([str(HERE/'build/native/rc'),'/help'],cwd=directory,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,pass_fds=(trace.fileno(),),timeout=10)
            execute=time.perf_counter()-execute
        verify=time.perf_counter()
        assert result.returncode==0,'Native status differs'
        assert result.stdout==(reference/'stdout.bin').read_bytes(),'Native stdout differs'
        assert result.stderr==(reference/'stderr.bin').read_bytes(),'Native stderr differs'
        assert (directory/'trace').read_text().splitlines()==['rc_main','rc_diagnostic'],'Native application entry trace differs'
        assert (directory/'boundary.log').read_text()=='RC 0 0 2\ncallback 0 0 0 0\ncallback 1 0 0 0\ncallback 2 0 0 0\ncallback 3 0 0 0\nreturn 0\n','Dependency call/callback order differs'
        assert sorted(p.name for p in directory.iterdir())==['boundary.log','filter','seen','trace'],'Unexpected fixture artifact'
        verify=time.perf_counter()-verify
    return {'status':'pass','stdout_bytes':len(result.stdout),'stderr_bytes':len(result.stderr),'setup_seconds':setup,'execution_seconds':execute,'verification_seconds':verify,'wine_invocations':0}

if __name__=='__main__':print(json.dumps(check(),indent=2))
