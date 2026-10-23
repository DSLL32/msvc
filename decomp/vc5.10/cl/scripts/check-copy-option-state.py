#!/usr/bin/env python3
"""Reject altered actual copy-option input fields and origin identity."""
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
assert expected['frontier']=='copy_option'
assert expected['parameters']['option']['name']=='Og'
assert expected['parameters']['option']['origin']['rule']['pattern']=='Og'
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset
            assert address not in memory or memory[address]==byte
            memory[address]=byte
    option=raw['context']['arguments'][0]
    def word(address):return struct.unpack('<I',bytes(memory[address+i]for i in range(4)))[0]
    def put(memory,address,value):
        for i,byte in enumerate(struct.pack('<I',value)):memory[address+i]=byte
    def negative(label,change):
        context=copy.deepcopy(raw['context']);changed=memory.copy();change(context,changed)
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),context)
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'input mutation ignored: '+label
        passed.append(platform+': '+label)
    negative('NULL option',lambda c,m:c['arguments'].__setitem__(0,0))
    negative('flags',lambda c,m:put(m,option+4,word(option+4)^1))
    negative('matching identity',lambda c,m:put(m,option+8,word(option+12)))
    negative('name bytes',lambda c,m:m.__setitem__(word(option+12),ord('X')))
    negative('value identity',lambda c,m:put(m,option+16,word(option+12)))
    negative('origin identity',lambda c,m:put(m,option+20,0))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
