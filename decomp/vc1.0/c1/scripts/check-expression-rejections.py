#!/usr/bin/env python3
"""Private expression-observation mutations; no target/original execution."""
import argparse
import json
from pathlib import Path
import re
import tempfile
import time
import importlib.util
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',type=Path)
    options = parser.parse_args()
    spec = importlib.util.spec_from_file_location('contract',Path(__file__).with_name('expression-contract.py'))
    contract = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(contract)
    start = time.perf_counter()
    root = Path.home()/'.ai-tmp/msvc2/vc1.0/c1/validation'
    directory = Path(tempfile.mkdtemp(prefix='expression-rejections-',dir=root))
    if options.baseline is None:
        options.baseline=directory/'baseline'
        subprocess.run([sys.executable,str(Path(__file__).with_name('check-expression-tests.py')),'--directory',str(options.baseline)],check=True,stdout=subprocess.DEVNULL)
    baseline = json.loads((options.baseline/'receipt.json').read_text())
    assert baseline['status']=='pass' and len(baseline['direct_units'])==6
    tests = []

    def reject(name,unit,change):
        row = contract.record((options.baseline/('%02d'%unit)/'report').read_text())
        change(row)
        began = time.perf_counter()
        try: contract.returned(row)
        except AssertionError as error:
            tests.append(dict(name=name,status='rejected',diagnostic=str(error),seconds=time.perf_counter()-began))
        else: raise AssertionError('invalid expression result accepted: '+name)

    def byte(row,name,offset):
        raw = bytearray(row['memory'][name]['data'])
        raw[offset] ^= 1
        row['memory'][name]['data'] = bytes(raw)

    reject('owned-source-payload',1,lambda r:byte(r,'input-before',12))
    reject('real-type-owner',4,lambda r:byte(r,'type-before-2',0))
    reject('changed-original-tree',3,lambda r:byte(r,'input-after',12))
    reject('wrong-child-parent',3,lambda r:byte(r,'slot-after',28))
    reject('aligned-string-tail-overwrite',4,lambda r:byte(r,'slot-after',25))
    reject('extended-value-word',5,lambda r:byte(r,'slot-after',24))
    reject('pool-transfer-counter',2,lambda r:byte(r,'pool-after',12))
    reject('zero-helper-unwritten-byte',0,lambda r:byte(r,'slot-after',2))
    def stack(row):
        row['text']=re.sub(r'(^expr-native-return [0-9a-f]+ )([0-9a-f]+)',lambda m:m[1]+'%08x'%(int(m[2],16)+4),row['text'],flags=re.M)
    reject('native-wrong-stack-cleanup',4,stack)
    reject('private-stack-canary',3,lambda r:r.__setitem__('text',r['text'].replace('expr-stack-guards a5','expr-stack-guards 00')))
    # The raw prior bytes beyond the allocator's proved60-byte clear are
    # preserved, rather than forced to zero or discarded as padding.
    row=contract.record((options.baseline/'04/report').read_text())
    byte(row,'slot-before',70);byte(row,'slot-after',70)
    contract.returned(row)
    tests.append(dict(name='unwritten-prior-byte-preserved',status='accepted exact prior-byte preservation'))
    receipt=dict(status='pass',tests=tests,elapsed_seconds=time.perf_counter()-start,directory=str(directory),policy='Private raw-observation copies only; no fixture writes, debugger, Wine or target invocation.')
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))


if __name__=='__main__':main()
