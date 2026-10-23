#!/usr/bin/env python3
"""Affected diagnostic formal/variadic/caller-alias controls on actual captures."""
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
assert expected['frontier']in ('warning','fatal','vwrite_message')
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset
            assert address not in memory or memory[address]==byte
            memory[address]=byte
    def negative(label,change):
        context=copy.deepcopy(raw['context']);changed=memory.copy();change(context,changed)
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),context)
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'changed diagnostic state ignored: '+label
        passed.append(platform+': '+label)
    def word(a):return struct.unpack('<I',bytes(memory[a+i]for i in range(4)))[0]
    negative('formal value',lambda c,m:c['arguments'].__setitem__(0,c['arguments'][0]^1))
    if expected['frontier']=='vwrite_message':
        negative('format bytes',lambda c,m:m.__setitem__(c['arguments'][1],ord('X')))
        negative('va_list first-slot alias',lambda c,m:c['arguments'].__setitem__(2,c['arguments'][2]+4))
        negative('diagnostic caller identity',lambda c,m:c['diagnostic_caller'].__setitem__('name','unknown'))
        negative('diagnostic caller id',lambda c,m:c['diagnostic_caller'].__setitem__('id',0))
        if expected['variable_arguments']:
            pointer=word(raw['context']['arguments'][2])
            negative('consumed variadic string',lambda c,m:m.__setitem__(pointer,ord('X')))
    elif expected['variable_arguments']:
        pointer=raw['context']['varargs'][0]
        negative('consumed variadic string',lambda c,m:m.__setitem__(pointer,ord('X')))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
