#!/usr/bin/env python3
"""Explicit fresh original/native acceptance trial; never invoked by fixtures."""
import argparse
import importlib.util
import json
import shutil
import time
from pathlib import Path
from common import HERE,read,original_trial_projection

def module(name,filename):
    spec=importlib.util.spec_from_file_location(name,HERE/'scripts'/filename)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reuse-original',type=Path);p.add_argument('--candidate',type=Path,default=HERE/'functions.json');p.add_argument('directory',type=Path);a=p.parse_args()
    started=time.perf_counter();timings={}
    a.directory.mkdir(parents=True,exist_ok=False)
    config=a.directory/'candidate.json';config.write_bytes(a.candidate.read_bytes())
    phase=time.perf_counter()
    module('rc_audit','audit.py').audit(config,a.directory/'audit.json')
    timings['audit_seconds']=time.perf_counter()-phase
    for source,name in [(HERE/'build/audit-original-runtime.json','audit-original-runtime.json'),(HERE/'build/runtime.json','audit-rebuilt-runtime.json'),(HERE/'build/rc.map','rc.map'),(HERE/'graph-metadata.json','graph-metadata.json')]:shutil.copyfile(source,a.directory/name)
    phase=time.perf_counter()
    if a.reuse_original:
        projection=original_trial_projection(read(config),read(a.reuse_original/'candidate.json'))
        shutil.copytree(a.reuse_original,a.directory/'original')
        attachment=a.directory/'original/compiled-sources.json'
        if not attachment.exists():
            import hashlib
            receipt=json.loads((a.reuse_original/'receipt.json').read_text())
            expected=dict(receipt['sources']);expected['scripts/debug-original.c']=receipt['debugger_source_sha256']
            contents={}
            for name,pin in expected.items():
                raw=(HERE/name).read_bytes();assert hashlib.sha256(raw).hexdigest()==pin,'Cannot attach historical source that has changed'
                contents[name]=raw.decode()
            attachment.write_text(json.dumps(contents,indent=2)+'\n')

        (a.directory/'reuse-original.json').write_text(json.dumps({'source':str(a.reuse_original.resolve()),'receipt_sha256':module('rc_common','common.py').sha(a.reuse_original/'receipt.json'),'driver_symbol_projection':projection,'reason':'Explicit reuse of unchanged target/module/case/candidate/mask inputs; native rebuilt independently'},indent=2)+'\n')
    else:
        module('rc_original','capture-original.py').capture(a.directory/'original',config)
    timings['original_reuse_copy_seconds' if a.reuse_original else 'original_capture_seconds']=time.perf_counter()-phase
    phase=time.perf_counter()
    module('rc_native','capture-native.py').capture(a.directory/'native',config)
    timings['native_capture_seconds']=time.perf_counter()-phase
    phase=time.perf_counter()
    result=module('rc_verify','verify-pair.py').validate(a.directory,config)
    (a.directory/'comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    timings['verification_seconds']=time.perf_counter()-phase
    timings['total_seconds']=time.perf_counter()-started
    (a.directory/'timings.json').write_text(json.dumps(timings,indent=2)+'\n')
    print(json.dumps(result,indent=2))
