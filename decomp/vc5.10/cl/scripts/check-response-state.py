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
assert expected['frontier']in ('configure_assembly','set_pass_name','undefine_defaults','enable_dll')
assert expected['globals']['response_depth']==0 and expected['globals']['response_first_line']==1
if expected['frontier']=='configure_assembly':
    assert expected['parameters']['option']['name']=='bA'and expected['parameters']['option']['value']=='2M a'
elif expected['frontier']=='set_pass_name':
    assert expected['parameters']['option']['name']=='bp'and expected['parameters']['option']['value']=='1c1.dll'
elif expected['frontier']=='undefine_defaults':
    assert expected['parameters']['option']['name']=='bu'and expected['parameters']['option']['value']is None
    assert expected['compiler_passes'][1]['name']=='c1.dll'and expected['compiler_passes'][3]['name']=='c2.exe'
else:
    assert expected['parameters']['option']['name']=='LD'and expected['parameters']['option']['value']is None
    assert expected['globals']['default_macros']==0
    assert not any(x.get('name')=='D'and x.get('flags',0)&1 for x in expected['globals']['driver']['options'])
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
    if word(option+16):negative('option value bytes',word(option+16),b'X')
    else:negative('unexpected option value',option+16,struct.pack('<I',1))
    negative('option origin alias',option+20,struct.pack('<I',0))
    if expected['frontier']in ('set_pass_name','undefine_defaults','enable_dll'):
        def string(address):
            value=bytearray()
            while memory[address]:value.append(memory[address]);address+=1
            return value.decode('ascii')
        node=word(word(raw['context']['globals']['driver']));nodes={}
        while node:
            nodes[string(word(node+12))]=node;node=word(node)
        assert string(word(nodes['FA']+16))=='acs'
        assert string(word(nodes['Fa']+16))=='response.asm'
        assert string(word(nodes['bA']+16))==''
        negative('assembly format bytes',word(nodes['FA']+16),b'X')
        negative('assembly filename bytes',word(nodes['Fa']+16),b'X')
        negative('assembly option consumed',word(nodes['bA']+16),b'X')
    if expected['frontier']in ('undefine_defaults','enable_dll'):
        base=raw['context']['compiler_passes_address']
        negative('C1 pass-name bytes',word(base+36),b'X')
        negative('C2 pass-name bytes',word(base+108),b'X')
    if expected['frontier']=='enable_dll':
        negative('default macro guard',raw['context']['globals']['default_macros'],struct.pack('<I',1))
        assert word(nodes['W']+4)&1
        negative('protected default macro survives',word(nodes['W']+12),b'D')
    path=directory/'options.rsp';old=path.read_bytes()
    try:
        path.write_bytes(old+b'/unexpected\n')
        try:project(platform,directory)
        except AssertionError:passed.append(platform+': source response bytes')
        else:raise AssertionError('changed response input ignored')
    finally:path.write_bytes(old)
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
