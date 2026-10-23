#!/usr/bin/env python3
"""Affected terminal FILE/descriptor/Win32 alias controls; no executables run."""
import argparse
import copy
import json
from pathlib import Path
import struct
import time
from dependency_provider import verify,encoded
from completion_state import decode
from dependency_response import decode as response_decode

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();started=time.perf_counter();response=response_decode(args.original)
assert response['completed'];verify(args.native)
pid=json.loads((args.native/'native-process.json').read_text())['pid'];passed=[]
def negative(phase,label,path,change):
    before=path.read_bytes()
    try:
        path.write_bytes(change(before))
        try:decode(args.native,phase,response,pid)
        except (AssertionError,KeyError,ValueError,struct.error):passed.append(phase+': '+label)
        else:raise AssertionError('terminal stream mutation accepted: '+phase+' '+label)
    finally:path.write_bytes(before)
for phase in ('postchild','exit'):
    iob=args.native/('streams-'+phase+'-iob.bin')
    for label,offset in (('stdin pointer',0),('stdin count',4),('stdin buffer alias',8),('stdout flags',44),('stderr descriptor',80),('stdin capacity',24),('temporary owner',92)):
        def word(value,offset=offset):
            changed=bytearray(value);struct.pack_into('<I',changed,offset,struct.unpack_from('<I',value,offset)[0]^1);return bytes(changed)
        negative(phase,label,iob,word)
    stream=args.native/('streams-'+phase+'.json')
    for label,change in (
        ('CRT handle',lambda x:x['crt'][1].update(handle=x['crt'][1]['handle']+1)),
        ('Win32 alias',lambda x:x['win32'][2].update(handle=x['win32'][2]['handle']+1)),
        ('file position',lambda x:x['crt'][1].update(position=x['crt'][1]['position']+1)),
        ('file destination',lambda x:x['win32'][1].update(inode=x['win32'][1]['inode']+1)),
        ('fd flags',lambda x:x['osfile'].__setitem__(2,0))):
        def mutate(value,change=change):
            fields=json.loads(value);change(fields);return encoded(fields)
        negative(phase,label,stream,mutate)
assert verify(args.native)['completed']
print(json.dumps({'result':'PASS','affected_negatives':len(passed),'seconds':time.perf_counter()-started,'controls':passed}))
