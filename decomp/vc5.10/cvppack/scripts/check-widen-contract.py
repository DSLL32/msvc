#!/usr/bin/env python3
"""Reject semantic dependency-capture mutations, with refreshed raw file hashes."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import time
from state import BINARY,sha

spec=importlib.util.spec_from_file_location('widen_contract',BINARY/'scripts/widen-contract.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
original=BINARY/'cases/hello-debug-dll/original-probe-0003'
start=time.perf_counter();decoded=module.decode(original)
assert decoded==json.loads((BINARY/'cases/hello-debug-dll/widen-contract.json').read_text())
rejected=[]
def reject(name,mutation):
    with tempfile.TemporaryDirectory(dir=Path.home()/'.ai-tmp') as tmp:
        directory=Path(tmp)/'capture';shutil.copytree(original,directory)
        receipt=json.loads((directory/'receipt.json').read_text())
        rows=(directory/'events').read_text().splitlines()
        index=next(i for i,row in enumerate(rows) if row.startswith('widen method-return 36 '))
        mutation(directory,receipt,rows,index)
        (directory/'events').write_text('\n'.join(rows)+'\n')
        for item in receipt['files']:
            path=directory/item['name'];item.update(sha256=sha(path),bytes=path.stat().st_size)
        (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
        # This deliberately proves the raw-container hashes are valid, so only
        # the newly evidenced byte/state contract can reject the mutation.
        for item in receipt['files']:assert sha(directory/item['name'])==item['sha256']
        try:module.decode(directory)
        except (AssertionError,KeyError):rejected.append(name)
        else:raise AssertionError('accepted '+name)
def mutate_byte(rows,index,last=False):
    parts=rows[index].split();raw=bytearray.fromhex(parts[2]);raw[-1 if last else 0]^=1;parts[2]=raw.hex();rows[index]=' '.join(parts)
reject('changed-post-emit-input',lambda d,r,rows,i:mutate_byte(rows,i+2))
reject('changed-own-output-capacity-tail',lambda d,r,rows,i:mutate_byte(rows,i+3,True))
def truncated(d,r,rows,i):
    parts=rows[i+3].split();parts[2]=parts[2][:-2];rows[i+3]=' '.join(parts)
reject('truncated-capacity-observation',truncated)
reject('missing-capacity-observation',lambda d,r,rows,i:rows.pop(i+3))
def wrong_provider(d,r,rows,i):
    path=d/'wrong-loaded-provider.dll';path.write_bytes(b'wrong loaded provider bytes')
    r['actual_loaded_mspdb']['path']=str(path)
    r['files'].append(dict(name=path.name,bytes=path.stat().st_size,sha256=sha(path)))
reject('wrong-actual-provider-despite-canonical-claim-and-refreshed-file-hash',wrong_provider)
report=dict(schema=1,native_only=True,positive_instances=21,all_raw_hashes_refreshed=True,rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/contract-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
