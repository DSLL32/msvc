#!/usr/bin/env python3
"""Explicit canonical-CRT original reference/frontier capture. Never a fixture helper."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import time

HERE=Path(__file__).resolve().parents[1]
ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('rc_wine',HERE/'scripts/wine-tool.py')
wt=importlib.util.module_from_spec(spec);spec.loader.exec_module(wt)
BASE=Path.home()/'.ai-tmp/msvc2/vc5.10/rc'
CRT_SHA='e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def capture(directory, candidate, baseline=False):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=False)
    settings=json.loads(Path(candidate).read_text())
    (directory/'candidate.json').write_bytes(Path(candidate).read_bytes())
    (directory/'runtime.json').write_bytes((HERE/'runtime.json').read_bytes())
    original=BASE/'original';original.mkdir(parents=True,exist_ok=True)
    for name in ['RC.EXE','RCDLL.DLL','MSVCRT.DLL']:
        src=ROOT/'toolchains/5.10/bin'/name;dest=original/name
        if dest.exists():assert sha(dest)==sha(src), 'Stale original dependency '+name
        else:shutil.copyfile(src,dest)
    assert sha(original/'MSVCRT.DLL')==CRT_SHA
    shutil.copyfile(HERE/'build/debug-original.exe',directory/'debug-original.exe')
    shutil.copyfile(original/'MSVCRT.DLL',directory/'MSVCRT.DLL')
    ranges=[]
    accepted=set(settings['accepted'])
    for row in settings['driver']+settings['support']+settings['imports']:
        if row['name'] in accepted:
            ranges.append((row['start']-0x1000000,row['end']-0x1000000,not row['name'].startswith('import:')))
    for row in settings.get('text_data',[]):
        if row['owner'] in accepted:ranges.append((row['start']-0x1000000,row['end']-0x1000000,2))
    (directory/'ranges.txt').write_text(''.join('%x %x %d\n'%r for r in ranges))
    command=['wine',str(directory/'debug-original.exe'),'baseline' if baseline else 'masked',wt.windows(original/'RC.EXE'),'ranges.txt']
    start=time.monotonic()
    run=subprocess.run(command,cwd=directory,env=wt.environment(),stdin=subprocess.DEVNULL,
                       stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=45)
    elapsed=time.monotonic()-start
    (directory/'stdout.bin').write_bytes(run.stdout);(directory/'stderr.bin').write_bytes(run.stderr)
    files={p.name:{'sha256':sha(p),'bytes':p.stat().st_size} for p in directory.iterdir() if p.is_file() and p.name not in ['MSVCRT.DLL','debug-original.exe']}
    source_snapshot={str(p.relative_to(HERE)):p.read_text() for p in sorted((HERE/'src').glob('*')) if p.is_file()}
    source_snapshot['scripts/debug-original.c']=(HERE/'scripts/debug-original.c').read_text()
    (directory/'compiled-sources.json').write_text(json.dumps(source_snapshot,indent=2)+'\n')
    files['compiled-sources.json']={'bytes':(directory/'compiled-sources.json').stat().st_size,'sha256':sha(directory/'compiled-sources.json')}
    receipt={'mode':'baseline' if baseline else 'masked','status':run.returncode,'elapsed_seconds':elapsed,
             'target_sha256':settings['target']['sha256'],'candidate_sha256':sha(candidate),
             'native_sha256':sha(HERE/'build/native/rc'),'rebuilt_sha256':sha(HERE/'build/rc.exe'),
             'map_sha256':sha(HERE/'build/rc.map'),'debugger_sha256':sha(directory/'debug-original.exe'),
             'debugger_source_sha256':sha(HERE/'scripts/debug-original.c'),
             'sources':{str(p.relative_to(HERE)):sha(p) for p in sorted((HERE/'src').glob('*')) if p.is_file()},
             'program':wt.windows(original/'RC.EXE'),'arguments':['/help'],
             'explicit_child_environment':True,'prefix':str(wt.PREFIX),'files':files}
    assert sha(directory/'loaded-target.bin')==settings['target']['sha256'],'Wrong actual RC module'
    assert sha(directory/'loaded-crt.bin')==CRT_SHA,'Wrong actual CRT module'
    assert sha(directory/'loaded-rcdll.bin')==sha(original/'RCDLL.DLL'),'Wrong actual RCDLL module'
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',action='store_true')
    parser.add_argument('--candidate',type=Path,default=HERE/'functions.json')
    parser.add_argument('directory',type=Path)
    args=parser.parse_args()
    print(json.dumps(capture(args.directory,args.candidate,args.baseline),indent=2))
