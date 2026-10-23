#!/usr/bin/env python3
"""Check typed pass/callback/current-input state against actual frontier bytes."""
import argparse
import copy
import json
from pathlib import Path
import shutil
import struct
import tempfile
from frontier_state import decode

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);args=parser.parse_args()
expected=decode(args.original,'original')['fields'];assert expected['frontier']=='run_passes'
raw=json.loads((args.original/'frontier-memory.json').read_text());passed=[]
with tempfile.TemporaryDirectory(prefix='job-state-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as tmp:
    directory=Path(tmp)
    for name in ['context.bin','events.txt','mb-state.bin','crt-state.bin','config.json']:shutil.copyfile(args.original/name,directory/name)
    def patch(value,address,word):
        seen=0
        for item in value['ranges']:
            data=bytearray.fromhex(item['data']);offset=address-item['address']
            if 0<=offset<=len(data)-4:struct.pack_into('<I',data,offset,word);item['data']=data.hex();seen+=1
        assert seen,'control did not touch captured bytes'
    def control(name,change):
        value=copy.deepcopy(raw);change(value)
        (directory/'frontier-memory.json').write_text(json.dumps(value))
        try:actual=decode(directory,'original')['fields']
        except (AssertionError,StopIteration):passed.append(name);return
        assert actual!=expected,'changed job state ignored: '+name
        passed.append(name)
    state=raw['context']['arguments'][1];passes=raw['context']['arguments'][0]
    control('unaligned compiler-pass record',lambda v:patch(v,state+8,v['context']['compiler_passes_address']+4))
    control('lost current-input alias',lambda v:patch(v,state+28,0))
    control('changed selected pass sequence',lambda v:patch(v,passes,4))
    def callback(value):
        value['context']['arguments'][2]=0
        data=bytearray((directory/'context.bin').read_bytes());struct.pack_into('<I',data,204+12,0);(directory/'context.bin').write_bytes(data)
    control('unknown callback identity',callback)
print('Actual job/pass/callback frontier matches; %d semantic mutation controls pass'%len(passed))
