#!/usr/bin/env python3
"""Actual conflict identifier, option spelling, origin and alias controls."""
import argparse
import copy
import json
from pathlib import Path
import struct
from frontier_state import decode,snapshot

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();expected=decode(args.original,'original')['fields']
assert decode(args.native,'native')['fields']==expected
assert expected['frontier']=='option_conflict'and expected['parameters']['id']==4025
assert expected['parameters']['old']['origin']['name']=='TC'
assert expected['parameters']['replacement']['name']=='TP'
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset;assert address not in memory or memory[address]==byte;memory[address]=byte
    def word(a):return struct.unpack('<I',bytes(memory[a+i]for i in range(4)))[0]
    def negative(label,change):
        context=copy.deepcopy(raw['context']);changed=memory.copy();change(context,changed)
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),context)
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'conflict input ignored: '+label
        passed.append(platform+': '+label)
    negative('message id',lambda c,m:c['arguments'].__setitem__(0,2016))
    negative('old option null',lambda c,m:c['arguments'].__setitem__(1,0))
    negative('replacement null',lambda c,m:c['arguments'].__setitem__(2,0))
    negative('replacement aliases old',lambda c,m:c['arguments'].__setitem__(2,c['arguments'][1]))
    old=raw['context']['arguments'][1];new=raw['context']['arguments'][2];origin=word(old+20)
    negative('old origin pointer',lambda c,m:m.__setitem__(old+20,m[old+20]^1))
    negative('origin spelling',lambda c,m:m.__setitem__(word(origin+12),ord('X')))
    negative('internal old value',lambda c,m:m.__setitem__(word(old+16),ord('X')))
    negative('replacement spelling',lambda c,m:m.__setitem__(word(new+12),ord('X')))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
