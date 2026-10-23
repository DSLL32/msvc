#!/usr/bin/env python3
"""Native reference/configuration rejection checks. No executable is launched."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import time
from case_paths import SCRATCH, canonical_parts, windows

BINARY=Path(__file__).resolve().parents[1]

def main():
    start=time.perf_counter()
    spec=importlib.util.spec_from_file_location('native_trace_rejections',Path(__file__).with_name('native-traces.py'))
    traces=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(traces)
    cache=json.loads(traces.CACHE.read_text())
    directory=Path(tempfile.mkdtemp(prefix='reference-rejections-',dir=SCRATCH/'vc1.0/link/native'))
    results=[]
    for label in ('wrong-entry','wrong-prefix','stale-reference','stale-metadata'):
        changed=copy.deepcopy(cache)
        if label=='wrong-entry':changed['cases']['/?']['expected'][0]='unregistered_application'
        elif label=='wrong-prefix':changed['accepted_application'][0]='unregistered_application'
        elif label=='stale-reference':changed['original_entry_references']['help']['receipt_sha256']='0'*64
        else:changed['metadata_sha256']['functions.json']='0'*64
        path=directory/(label+'.json')
        path.write_text(json.dumps(changed))
        try:
            traces.load(BINARY/'build/native-trace',path)
        except AssertionError as error:
            results.append(dict(test=label,status='rejected',error=str(error),processes_launched=0))
        else:raise RuntimeError('Accepted invalid cache: '+label)
    for label,operation in [('undefined-capture-drive',lambda:windows('/tmp/unregistered')),
                            ('undefined-native-drive',lambda:canonical_parts(r'Q:\unregistered'))]:
        try:operation()
        except (AssertionError,ValueError) as error:
            results.append(dict(test=label,status='rejected',error=str(error),processes_launched=0))
        else:raise RuntimeError('Accepted invalid path: '+label)
    receipt=dict(status='pass',tests=results,total_seconds=time.perf_counter()-start)
    (directory/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(directory/'receipt.json')

if __name__=='__main__':main()
