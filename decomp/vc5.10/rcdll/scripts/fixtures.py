#!/usr/bin/env python3
"""Native-only regression: cached original streams, callback effects and entries."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from protocol import HERE,ROOT,callbacks,sha,logical_trace,LOGICAL_NAME_ALIASES,native_name

def run():
    started=time.perf_counter();reference=HERE/'cases/help/reference'
    manifest=json.loads((reference/'manifest.json').read_text())
    for name,digest in manifest['files'].items():assert sha(reference/name)==digest,'changed cached reference '+name
    assert sha(ROOT/'toolchains/5.10/bin/RCDLL.DLL')==manifest['target_sha256']
    assert sha(ROOT/'toolchains/5.10/bin/MSVCRT.DLL')==manifest['crt_sha256']
    native=HERE/'build/native/rcdll-host';config=json.loads((HERE/'functions.json').read_text())
    names=[f['name']for f in config['functions']if f['kind']=='application' and f['name'] in config['accepted']]
    with tempfile.TemporaryDirectory(prefix='help-fixture-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/rcdll')as scratch:
        out=Path(scratch);(out/'filter.txt').write_text(''.join(native_name(name)+'\n'for name in names));(out/'seen.txt').write_text('')
        env=os.environ.copy()
        for key in ['WINEPREFIX','WINEARCH','WINEDLLOVERRIDES','WINEPATH','RCDLL_NEGATIVE_MODULE_SHA','RECON_TRACE_STATE_FD','RECON_RC_ENVIRONMENT']:env.pop(key,None)
        env.update(RECON_RC_ARGV0=manifest['argv0'],RECON_RC_DLL_PATH=manifest['module_path'],RECON_RC_RESOURCES=str(HERE/'build/help-resources.bin'),RECON_TRACE_FILTER=str(out/'filter.txt'),RECON_TRACE_SEEN=str(out/'seen.txt'))
        setup=time.perf_counter()-started
        with (out/'trace.txt').open('wb')as trace:
            env['RECON_TRACE_FD']=str(trace.fileno());before=time.perf_counter()
            result=subprocess.run([str(native),str(out/'events.txt')],cwd=HERE,env=env,capture_output=True,timeout=10,pass_fds=(trace.fileno(),))
            execution=time.perf_counter()-before
        before=time.perf_counter()
        assert result.returncode==manifest['status']==0,'native status differs'
        assert result.stdout==(reference/'stdout.bin').read_bytes(),'stdout differs'
        assert result.stderr==(reference/'stderr.bin').read_bytes(),'stderr differs'
        assert logical_trace((out/'trace.txt').read_text())==logical_trace((reference/'trace.txt').read_text()),'application entry trace differs'
        assert callbacks(out)==json.loads((reference/'callbacks.json').read_text()),'diagnostic callback effects differ'
        events=(out/'events.txt').read_text().splitlines()
        assert events[0:2]==['attach','export-call RC 0 0 callback 0 2']
        assert events[-3:]==['export-return 0','detach','detach-return 1']
        checks=time.perf_counter()-before
    report=dict(logical_name_aliases=LOGICAL_NAME_ALIASES,case='help',status='pass',stdout_bytes=len(result.stdout),application_entries=len(logical_trace((reference/'trace.txt').read_text())),seconds=dict(setup=setup,execution=execution,checks=checks,total=time.perf_counter()-started),native_sha256=sha(native),reference_manifest_sha256=sha(reference/'manifest.json'))
    (HERE/'build/fixture-timing.json').write_text(json.dumps(report,indent=2)+'\n')
    return report
if __name__=='__main__':print(json.dumps(run(),indent=2))
