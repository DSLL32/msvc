#!/usr/bin/env python3
"""Check observed linker-driver inputs without relying on trap identity alone."""
import argparse
import copy
import json
from pathlib import Path
import struct
from frontier_state import decode,snapshot

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();expected=decode(args.original,'original')['fields']
assert expected==decode(args.native,'native')['fields']
assert expected['frontier'] in ('link_pass','write_link_response')
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset
            assert address not in memory or memory[address]==byte
            memory[address]=byte
    def word(address):return struct.unpack('<I',bytes(memory[address+i]for i in range(4)))[0]
    def put(memory,address,value):
        for i,byte in enumerate(struct.pack('<I',value)):memory[address+i]=byte
    def negative(label,change):
        context=copy.deepcopy(raw['context']);changed=memory.copy();change(context,changed)
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),context)
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'linker input mutation ignored: '+label
        passed.append(platform+': '+label)
    driver=raw['context']['arguments'][0];compiler_pass=word(driver+8)
    negative('NULL driver formal',lambda c,m:c['arguments'].__setitem__(0,0))
    negative('option list',lambda c,m:put(m,driver,0))
    negative('input list',lambda c,m:put(m,driver+12,0))
    negative('current pass',lambda c,m:put(m,driver+8,0))
    negative('pass name bytes',lambda c,m:m.__setitem__(word(compiler_pass),ord('X')))
    negative('pass flags',lambda c,m:put(m,compiler_pass+16,word(compiler_pass+16)^1))
    negative('temporary filename bytes',lambda c,m:m.__setitem__(word(driver+32),ord('X')))
    negative('output-produced state',lambda c,m:put(m,c['globals']['output_produced'],word(c['globals']['output_produced'])^1))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
