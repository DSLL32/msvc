#!/usr/bin/env python3
"""Cached native-only assembler support units; no source reachability claim."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
BINARY=Path(__file__).resolve().parents[1]
def load_contract():
    spec=importlib.util.spec_from_file_location('c1_direct_contract',Path(__file__).with_name('direct-contract.py'))
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',type=Path)
    parser.add_argument('--exe',type=Path,default=BINARY/'build/native/C13232.EXE')
    options=parser.parse_args();start=time.perf_counter();case=BINARY/'cases/asm-direct-callbacks';manifest=json.loads((case/'manifest.json').read_text());cfg=json.loads((BINARY/'functions.json').read_text());module=load_contract()
    root=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation';root.mkdir(parents=True,exist_ok=True)
    directory=options.directory or Path(tempfile.mkdtemp(prefix='direct-check-',dir=root))
    if options.directory:directory.mkdir(parents=True,exist_ok=False)
    reports=[]
    for i,row in enumerate(manifest['direct_oracles']):
        original=case/row['directory']
        actual={str(p.relative_to(original)):hashlib.sha256(p.read_bytes()).hexdigest()for p in original.rglob('*')if p.is_file()}
        assert actual==row['files'],'direct cached original file set/hash differs'
        assert row['input']==manifest['direct_plan'][i],'direct invocation differs from frozen plan'
        target=directory/('%02d'%i)
        command=[sys.executable,str(BINARY/'scripts/direct-native.py'),'--function',row['input']['function'][1:].split('@')[0],'--arguments',*[str(v)for v in row['input']['arguments']],'--directory',str(target),'--body','--exe',str(options.exe.resolve())]
        setup=time.perf_counter();result=subprocess.run(command,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=35);(directory/('%02d.host.log'%i)).write_bytes(result.stdout);assert not result.returncode,result.stdout.decode('latin1');check=time.perf_counter()
        matched=module.validate(original,target,dict(logical=row['input']['function'],arguments=row['input']['arguments']),cfg);(target/'comparison.json').write_text(json.dumps(matched,indent=2)+'\n')
        receipt=json.loads((target/'receipt.json').read_text());reports.append(dict(index=i,input=row['input'],status='pass',setup_seconds=receipt['setup_seconds'],execution_seconds=receipt['execution_seconds'],host_run_seconds=check-setup,check_seconds=time.perf_counter()-check,defined_result=matched['defined_result']))
    receipt=dict(status='pass',public_reachability=False,direct_units=reports,elapsed_seconds=time.perf_counter()-start,execution_policy='Only native i386 ELF/GDB with cached original bytes and explicit direct units; no Wine/original capture',directory=str(directory));(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('Direct assembler support:',len(reports),'passed',receipt['elapsed_seconds'],'seconds; no public source reachability claim')
if __name__=='__main__':main()
