#!/usr/bin/env python3
"""Native external expression units; no Wine, debugger or reference refresh."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
from case_paths import native_work,windows
BINARY=Path(__file__).resolve().parents[1]
ROOT=BINARY.parents[2]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def prepare(unit,directory):
    start=time.perf_counter();directory=Path(directory).resolve();directory.mkdir(parents=True,exist_ok=False)
    case=BINARY/'cases/expression-templates';manifest=json.loads((case/'manifest.json').read_text());phase=manifest['phases']['context'];work,paths=native_work(directory,phase['canonical_working_directory'])
    assert sha(ROOT/phase['target'])==phase['sha256'],'original target identity differs'
    for item in phase['inputs']:
        path=case/item['path'];assert sha(path)==item['sha256'];(work/path.name).write_bytes(path.read_bytes())
    for item in phase['toolchain_dependencies']:assert sha(ROOT/item['path'])==item['sha256']
    env=dict(LC_ALL='C',TZ='PST8PDT',RECON_PROGRAM_PATH=windows(ROOT/phase['target']),MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin'),include=windows(ROOT/'toolchains/1.0-nt/include')));env.update(paths)
    return directory,work,env,time.perf_counter()-start

def run(unit,directory,executable=None):
    start=time.perf_counter();directory,work,env,setup=prepare(unit,directory);executable=Path(executable or BINARY/'build/expression-tests/C13232.EXE').resolve();command=[str(executable),str(unit),str(directory/'report')];began=time.perf_counter()
    with(directory/'stdout').open('wb')as output:r=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=output,stderr=subprocess.STDOUT,timeout=30)
    (directory/'stderr').write_bytes(b'');receipt=dict(unit=unit,exit=r.returncode,setup_seconds=setup,execution_seconds=time.perf_counter()-began,total_seconds=time.perf_counter()-start,command=command,environment=env,executable_sha256=sha(executable),public_reachability=False,policy='Native external harness only; initialized marker and controlled ucontext stack. No Wine, debugger or reference refresh.',work=str(work));(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');return receipt

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unit',type=int,choices=range(6),required=True);p.add_argument('--directory',type=Path,required=True);p.add_argument('--executable',type=Path);args=p.parse_args();print(json.dumps(run(args.unit,args.directory,args.executable),indent=2))
if __name__=='__main__':main()
