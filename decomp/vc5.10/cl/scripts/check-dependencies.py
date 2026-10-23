#!/usr/bin/env python3
"""Reject changed owned compiler inputs without executing their PE bytes."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from case_inputs import verify_dependencies

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture',type=Path);args=parser.parse_args()
proof=verify_dependencies(args.capture);passed=[]
with tempfile.TemporaryDirectory(prefix='dependency-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as tmp:
    directory=Path(tmp)
    def control(name,change):
        for file in directory.iterdir():file.unlink()
        for file in ['dependency-inputs.json']+[item['name']for item in proof['files']]:shutil.copyfile(args.capture/file,directory/file)
        change(directory)
        try:verify_dependencies(directory)
        except (AssertionError,FileNotFoundError):passed.append(name);return
        raise AssertionError('changed compiler dependency accepted: '+name)
    def metadata(directory,key,value):
        path=directory/'dependency-inputs.json';data=json.loads(path.read_text());data['files'][0][key]=value;path.write_text(json.dumps(data))
    def bytes_changed(directory):
        path=directory/proof['files'][0]['name'];raw=bytearray(path.read_bytes());raw[-1]^=1;path.write_bytes(raw)
    control('missing frontend',lambda d:(d/proof['files'][0]['name']).unlink())
    control('changed frontend bytes',bytes_changed)
    control('unknown dependency identity',lambda d:metadata(d,'name','other.dll'))
    control('changed declared digest',lambda d:metadata(d,'sha256','0'*64))
    control('changed provenance path',lambda d:metadata(d,'source','toolchains/other/C1.DLL'))
print('Owned inert compiler inputs match; %d dependency mutation controls pass'%len(passed))
