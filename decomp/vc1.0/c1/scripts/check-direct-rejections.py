#!/usr/bin/env python3
"""Private mutations of direct callback results; original cache stays unchanged."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
B=Path(__file__).resolve().parents[1]
def main():
    start=time.perf_counter();root=Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation';directory=Path(tempfile.mkdtemp(prefix='direct-rejections-',dir=root));case=B/'cases/asm-direct-callbacks';manifest=json.loads((case/'manifest.json').read_text());cfg=json.loads((B/'functions.json').read_text())
    sp=importlib.util.spec_from_file_location('direct_contract',Path(__file__).with_name('direct-contract.py'));module=importlib.util.module_from_spec(sp);sp.loader.exec_module(module)
    # The baseline is one newly executed frozen-ELF direct unit suite, provided
    # by the caller's latest stable receipt; no original invocation is permitted.
    import argparse
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--baseline',type=Path);options=parser.parse_args()
    if options.baseline is None:
        options.baseline=directory/'baseline'
        subprocess.run([sys.executable,str(B/'scripts/check-direct.py'),'--directory',str(options.baseline)],cwd=B,check=True,stdout=subprocess.DEVNULL)
    baseline=json.loads((options.baseline/'receipt.json').read_text());assert baseline['status']=='pass'and len(baseline['direct_units'])==15
    tests=[]
    mutations=[('wrong-current-token',1,lambda s:s['before'].__setitem__('asm_current_token',33)),('wrong-error-count',1,lambda s:s['after'].__setitem__('error_count',3)),('wrong-defined-zero',4,lambda s:s['after']['registers'].__setitem__('eax',1)),('wrong-type-owner',8,lambda s:s['after']['registers'].__setitem__('edx',0)),('changed-nonvolatile',1,lambda s:s['after']['registers'].__setitem__('ebx',s['after']['registers']['ebx']^1)),('wrong-stack-cleanup',1,lambda s:s['after']['registers'].__setitem__('esp',s['after']['registers']['esp']+4)),('stack-canary-corruption',1,lambda s:s.__setitem__('canary','00'*16))]
    for label,index,mutate in mutations:
        target=directory/label;shutil.copytree(options.baseline/('%02d'%index),target);state=json.loads((target/'state.json').read_text());mutate(state);(target/'state.json').write_text(json.dumps(state));row=manifest['direct_oracles'][index];t=time.perf_counter()
        try:module.validate(case/row['directory'],target,dict(logical=row['input']['function'],arguments=row['input']['arguments']),cfg)
        except AssertionError as error:tests.append(dict(test=label,status='rejected',diagnostic=str(error),seconds=time.perf_counter()-t))
        else:raise AssertionError('Invalid direct result accepted: '+label)
    # Positive definedness control: original instructions never write EDX for
    # token39. Vary its retained raw unspecified word without inventing a value.
    target=directory/'undefined-word-control';shutil.copytree(options.baseline/'04',target);state=json.loads((target/'state.json').read_text());state['after']['registers']['edx']^=0x5a5aa5a5;(target/'state.json').write_text(json.dumps(state));row=manifest['direct_oracles'][4];module.validate(case/row['directory'],target,dict(logical=row['input']['function'],arguments=row['input']['arguments']),cfg);tests.append(dict(test='original-unwritten-word-control',status='accepted as unspecified',provenance='Original4c9b3 writes lowword only; not an arbitrary ignored-output field'))
    receipt=dict(status='pass',tests=tests,elapsed_seconds=time.perf_counter()-start,policy='Private result mutations only; no cache writes, Wine or original execution',directory=str(directory));(directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
if __name__=='__main__':main()
