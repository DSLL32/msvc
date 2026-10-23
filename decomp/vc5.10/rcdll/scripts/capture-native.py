#!/usr/bin/env python3
"""Linux-only debugger capture. No original runner or compiler dependency."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
HERE=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--argv0',required=True);p.add_argument('--module-path',required=True);p.add_argument('--negative-module-sha');p.add_argument('--configuration',type=Path)
    a=p.parse_args();out=a.output.resolve()
    if out.exists():p.error('output must be new')
    out.mkdir(parents=True)
    from protocol import candidate,validation_versions
    selected=a.configuration or HERE/'functions.json'
    execution=candidate();execution['files']['decomp/vc5.10/rcdll/functions.json']=sha(selected)
    (out/'candidate.json').write_text(json.dumps(execution,indent=2)+'\n')
    (out/'validation-versions.json').write_text(json.dumps(validation_versions(),indent=2)+'\n')
    config=selected;native=HERE/'build/native/rcdll-host'
    (out/'captured-native.elf').write_bytes(native.read_bytes())
    (out/'compiled-source.json').write_bytes((HERE/'build/native-source.json').read_bytes())
    (out/'functions.json').write_bytes(config.read_bytes())
    (out/'help-resources.bin').write_bytes((HERE/'build/help-resources.bin').read_bytes())
    (out/'data.json').write_bytes((HERE/'data.json').read_bytes())
    env=os.environ.copy();env.update(RCDLL_NATIVE_SNAPSHOT=str(out/'snapshot.json'),RCDLL_NATIVE_CONFIG=str(out/'functions.json'),RCDLL_NATIVE_SCRIPT=str(HERE/'scripts/native-gdb.py'),RECON_RC_ARGV0=a.argv0,RECON_RC_DLL_PATH=a.module_path,RECON_RC_RESOURCES=str(HERE/'build/help-resources.bin'))
    if a.negative_module_sha:env['RCDLL_NEGATIVE_MODULE_SHA']=a.negative_module_sha
    else:env.pop('RCDLL_NEGATIVE_MODULE_SHA',None)
    for k in ['WINEPREFIX','WINEARCH','WINEDLLOVERRIDES','WINEPATH']:env.pop(k,None)
    command=['gdb','--batch','-q',str(native),'-ex','set pagination off','-ex','handle SIGTRAP stop print nopass','-ex','set breakpoint pending on','-ex','source '+str(HERE/'scripts/native-observers.py'),'-ex','run '+str(out/'events.txt')+' > '+str(out/'stdout.bin')+' 2> '+str(out/'stderr.bin'),'-ex','source '+str(HERE/'scripts/native-gdb.py')]
    active=json.loads(config.read_text());names=[f['name']for f in active['functions']if f['kind']=='application' and f['name'] in active['accepted']+[active.get('trial_function')]]
    (out/'trace-filter.txt').write_text(''.join(name+'\n'for name in names));(out/'trace-seen.txt').write_text('')
    with (out/'trace.txt').open('wb') as trace:
        env.update(RECON_TRACE_FD=str(trace.fileno()),RECON_TRACE_FILTER=str(out/'trace-filter.txt'),RECON_TRACE_SEEN=str(out/'trace-seen.txt'))
        started=time.perf_counter();result=subprocess.run(command,cwd=HERE,env=env,capture_output=True,timeout=15,pass_fds=(trace.fileno(),));elapsed=time.perf_counter()-started
    (out/'gdb.stdout').write_bytes(result.stdout);(out/'gdb.stderr').write_bytes(result.stderr)
    report=dict(status=result.returncode,elapsed_seconds=elapsed,native_sha256=sha(native),candidate_sha256=sha(config),command=command,files={f.name:dict(bytes=f.stat().st_size,sha256=sha(f)) for f in out.iterdir() if f.is_file()})
    (out/'receipt.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(json.loads((out/'snapshot.json').read_text()),indent=2));return result.returncode
if __name__=='__main__':raise SystemExit(main())
