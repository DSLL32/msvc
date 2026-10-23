#!/usr/bin/env python3
"""Affected option and compiler-pass state controls on actual debugger bytes."""
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
assert expected['frontier']in ('enable_pass','toggle_pass','set_pass_suffix')
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset;assert address not in memory or memory[address]==byte;memory[address]=byte
    def word(a):return struct.unpack('<I',bytes(memory[a+i]for i in range(4)))[0]
    def negative(label,address):
        changed=memory.copy();changed[address]^=1
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),copy.deepcopy(raw['context']))
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'pass control mutation ignored: '+label
        passed.append(platform+': '+label)
    option=raw['context']['arguments'][0]
    negative('option flags',option+4)
    negative('option selector/value',word(option+16))
    negative('option origin',option+20)
    base=raw['context']['compiler_passes_address']
    for index in range(5):
        negative('pass%d flags'%index,base+36*index+16)
        negative('pass%d selector'%index,base+36*index+12)
        if word(base+36*index+32):negative('pass%d leading argument bytes'%index,word(base+36*index+32))
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
