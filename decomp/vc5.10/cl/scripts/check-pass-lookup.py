#!/usr/bin/env python3
"""Reject changed located compiler identity, type flags and DLL slots."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import struct
import tempfile
from frontier_state import decode
from case_inputs import verify_dependencies

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);args=parser.parse_args()
expected=decode(args.original,'original')['fields'];actual=decode(args.native,'native')['fields']
assert expected==actual and expected['frontier']=='final_pass'
proof=verify_dependencies(args.original);assert proof==verify_dependencies(args.native)
assert any(item['name']=='C1.DLL'for item in proof['files'])
selected=expected['globals']['driver']['current_pass']['index'];assert selected==1
assert expected['compiler_passes'][selected]['flags']==15
assert expected['compiler_passes'][selected]['name'].lower().endswith('\\c1.dll')
raw=json.loads((args.original/'frontier-memory.json').read_text());passed=[]
with tempfile.TemporaryDirectory(prefix='pass-lookup-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as tmp:
    directory=Path(tmp)
    for name in ['context.bin','events.txt','mb-state.bin','crt-state.bin','config.json']:shutil.copyfile(args.original/name,directory/name)
    def patch(value,address,word):
        seen=0
        for item in value['ranges']:
            data=bytearray.fromhex(item['data']);offset=address-item['address']
            if 0<=offset<=len(data)-4:struct.pack_into('<I',data,offset,word);item['data']=data.hex();seen+=1
        assert seen
    def control(name,change):
        value=copy.deepcopy(raw);change(value);(directory/'frontier-memory.json').write_text(json.dumps(value))
        try:result=decode(directory,'original')['fields']
        except (AssertionError,StopIteration):passed.append(name);return
        assert result!=expected,'changed located pass ignored: '+name
        passed.append(name)
    base=raw['context']['compiler_passes_address'];record=base+36*selected
    state=raw['context']['arguments'][0]
    control('different current compiler',lambda v:patch(v,state+8,base+72))
    control('missing DLL classification',lambda v:patch(v,record+16,11))
    control('missing last-input flag',lambda v:patch(v,record+16,7))
    control('unknown successful DLL module',lambda v:patch(v,record+20,1))
    control('unknown Invoke pointer',lambda v:patch(v,record+24,1))
print('Actual located C1 pass and owned dependencies match; %d mutation controls pass'%len(passed))
