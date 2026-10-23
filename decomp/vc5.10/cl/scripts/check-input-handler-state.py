#!/usr/bin/env python3
"""Affected explicit input handler formals and consumed memory controls."""
import argparse
import copy
import json
from pathlib import Path
import struct
from frontier_state import decode,snapshot,PARAMETERS

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();expected=decode(args.original,'original')['fields']
assert decode(args.native,'native')['fields']==expected
assert expected['frontier']in ('typed_input','c_input','cpp_input','object_input','link_arguments')
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset;assert address not in memory or memory[address]==byte;memory[address]=byte
    def negative(label,change):
        context=copy.deepcopy(raw['context']);changed=memory.copy();change(context,changed)
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),context)
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'input handler mutation ignored: '+label
        passed.append(platform+': '+label)
    def word(a):return struct.unpack('<I',bytes(memory[a+i]for i in range(4)))[0]
    parameters=PARAMETERS[expected['frontier']]
    for slot,(name,kind)in enumerate(parameters):
        if kind=='signed':negative(name,lambda c,m,slot=slot:c['arguments'].__setitem__(slot,c['arguments'][slot]+1))
        else:negative(name+' null pointer',lambda c,m,slot=slot:c['arguments'].__setitem__(slot,0))
    positions={name:i for i,(name,kind)in enumerate(parameters)}
    pointer=raw['context']['arguments'][positions['index']]
    negative('actual index word',lambda c,m:m.__setitem__(pointer,m[pointer]^1))
    vector=raw['context']['arguments'][positions['args']]
    current=word(pointer);text=word(vector+4*current)
    negative('actual current option bytes',lambda c,m:m.__setitem__(text,ord('X')))
    if 'option'in positions:
        pointer=raw['context']['arguments'][positions['option']]
        negative('explicit input prefix bytes',lambda c,m:m.__setitem__(pointer,ord('X')))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
