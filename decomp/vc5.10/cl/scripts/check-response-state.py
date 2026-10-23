#!/usr/bin/env python3
"""Affected response-reader scalar/option/input controls, from actual memory."""
import argparse
import copy
import json
from pathlib import Path
import struct
from frontier_state import decode,snapshot
from state import project

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();expected=decode(args.original,'original')['fields']
assert decode(args.native,'native')['fields']==expected
assert expected['frontier']=='configure_assembly'
assert expected['globals']['response_depth']==0 and expected['globals']['response_first_line']==1
assert expected['parameters']['option']['name']=='bA'and expected['parameters']['option']['value']=='2M a'
passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    raw=json.loads((directory/'frontier-memory.json').read_text());memory={}
    for item in raw['ranges']:
        for offset,byte in enumerate(bytes.fromhex(item['data'])):
            address=item['address']+offset;assert address not in memory or memory[address]==byte;memory[address]=byte
    def negative(label,address,value):
        changed=memory.copy()
        for offset,byte in enumerate(value):changed[address+offset]=byte
        try:result=snapshot(lambda a,n:bytes(changed[a+i]for i in range(n)),copy.deepcopy(raw['context']))
        except (AssertionError,KeyError):passed.append(platform+': '+label);return
        assert result!=expected,'changed response state ignored: '+label
        passed.append(platform+': '+label)
    word=lambda a:struct.unpack('<I',bytes(memory[a+i]for i in range(4)))[0]
    for name,value in (('response_depth',1),('response_first_line',0),('no_logo',0),('echo_response',1)):
        negative(name,raw['context']['globals'][name],struct.pack('<I',value))
    option=raw['context']['arguments'][0]
    negative('option flags',option+4,struct.pack('<I',1))
    negative('option value bytes',word(option+16),b'X')
    negative('option origin alias',option+20,struct.pack('<I',0))
    path=directory/'options.rsp';old=path.read_bytes()
    try:
        path.write_bytes(old+b'/unexpected\n')
        try:project(platform,directory)
        except AssertionError:passed.append(platform+': source response bytes')
        else:raise AssertionError('changed response input ignored')
    finally:path.write_bytes(old)
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
