#!/usr/bin/env python3
"""Reject changes to the response stream, format and actual variadic input."""
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
assert expected['frontier']=='write_link_message' and expected['parameters']['text']=='%c'
assert expected['variable_arguments']==[ord('/')]
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
        except (AssertionError,KeyError,TypeError):passed.append(platform+': '+label);return
        assert result!=expected,'response input mutation ignored: '+label
        passed.append(platform+': '+label)
    negative('NULL FILE argument',lambda c,m:c['arguments'].__setitem__(0,0))
    negative('FILE descriptor',lambda c,m:m.__setitem__(c['arguments'][0]+16,m[c['arguments'][0]+16]^1))
    negative('NULL format argument',lambda c,m:c['arguments'].__setitem__(1,0))
    negative('format byte',lambda c,m:m.__setitem__(c['arguments'][1],ord('X')))
    negative('consumed variadic character',lambda c,m:c['varargs'].__setitem__(0,ord('-')))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
