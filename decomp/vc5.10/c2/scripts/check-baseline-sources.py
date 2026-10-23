#!/usr/bin/env python3
"""Reject missing, changed, extra or escaping ordinary baseline source inputs."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import time
from state import BINARY,sha

spec=importlib.util.spec_from_file_location('c2_rejection_controls',BINARY/'scripts/check-rejections.py')
controls=importlib.util.module_from_spec(spec);spec.loader.exec_module(controls)

def main():
    import argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();begin=time.monotonic()
    old=BINARY/'cases/rejection-protocol/0066/baseline.json';controls.baseline_contracts(old,json.loads(old.read_text()))
    rejects=[]
    with tempfile.TemporaryDirectory(prefix='baseline-source-controls-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/c2')as folder:
        root=Path(folder);source=root/'sources/scripts';source.mkdir(parents=True)
        for name in ('compare.py','check-rejections.py'):shutil.copyfile(BINARY/'scripts'/name,source/name)
        mapping={str(p.relative_to(root/'sources')):sha(p.read_bytes())for p in source.iterdir()}
        raw=json.dumps(mapping,sort_keys=True).encode();(root/'manifest.json').write_bytes(raw)
        value={'source_directory':'sources','source_manifest':'manifest.json','source_manifest_sha256':sha(raw),'checker_contracts':mapping}
        assert controls.baseline_contracts(root/'baseline.json',value)==mapping
        operations=[('missing source',lambda r,v:(r/'sources/scripts/compare.py').unlink()),
            ('changed source',lambda r,v:(r/'sources/scripts/compare.py').write_bytes(b'changed')),
            ('extra source',lambda r,v:(r/'sources/extra.c').write_bytes(b'extra')),
            ('missing manifest',lambda r,v:(r/'manifest.json').unlink()),
            ('changed manifest hash',lambda r,v:v.__setitem__('source_manifest_sha256','0'*64)),
            ('directory escapes baseline',lambda r,v:v.__setitem__('source_directory','../sources')),
            ('manifest escapes baseline',lambda r,v:v.__setitem__('source_manifest','../manifest.json')),
            ('symlink source',lambda r,v:((r/'sources/scripts/compare.py').unlink(),(r/'sources/scripts/compare.py').symlink_to(BINARY/'scripts/compare.py')))]
        def traversal(r,v):
            bad={'../compare.py':mapping['scripts/compare.py']};data=json.dumps(bad).encode();(r/'manifest.json').write_bytes(data);v.update(checker_contracts=bad,source_manifest_sha256=sha(data))
        operations.append(('manifest filename traversal',traversal))
        for i,(name,mutate)in enumerate(operations):
            temp=root/str(i);shutil.copytree(root/'sources',temp/'sources');shutil.copyfile(root/'manifest.json',temp/'manifest.json');trial=copy.deepcopy(value);mutate(temp,trial)
            try:controls.baseline_contracts(temp/'baseline.json',trial)
            except (AssertionError,FileNotFoundError):rejects.append(name)
            else:raise AssertionError('baseline mutation accepted: '+name)
    a.output.write_text(json.dumps({'legacy_baseline':'PASS','ordinary_control':'PASS','rejected':rejects,'count':len(rejects),'seconds':time.monotonic()-begin,'checker_sha256':sha((BINARY/'scripts/check-rejections.py').read_bytes())},indent=2)+'\n')

if __name__=='__main__':main()
