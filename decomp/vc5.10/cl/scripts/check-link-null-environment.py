#!/usr/bin/env python3
"""Keep the old C1/C2 assignment prefix mandatory while LINK permits NULL."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tempfile
from dependency_response import decode
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('minimal_original',type=Path);args=parser.parse_args()
expected=decode(args.minimal_original);passed=[]
with tempfile.TemporaryDirectory(prefix='old-pass-environment-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary)/'raw';shutil.copytree(args.minimal_original,directory)
    saved=(directory/'capture.json').read_bytes()
    for phase in ('invoke-before','spawn-before'):
        name='observer/'+phase+'-caller-pass.bin';path=directory/name;original=path.read_bytes();raw=bytearray(original);struct.pack_into('<I',raw,8,0);path.write_bytes(raw)
        capture=json.loads(saved);capture['files'][name]={'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()};(directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
        try:
            try:decode(directory)
            except AssertionError as error:
                assert 'pass environment must be non-NULL and exact'in str(error);passed.append(phase)
            else:raise AssertionError('accepted NULL environment for '+phase)
        finally:path.write_bytes(original);(directory/'capture.json').write_bytes(saved)
assert decode(args.minimal_original)==expected
print(json.dumps({'passed':True,'negative_controls':len(passed),'controls':passed,'scope':'Original raw unchanged; offline decoder only'}))
