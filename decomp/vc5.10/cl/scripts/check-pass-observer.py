#!/usr/bin/env python3
"""Affected real-entry table, string identity, bounds and phase controls."""
import argparse
import json
from pathlib import Path
import struct
from pass_state import decode,expected

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('original',type=Path);parser.add_argument('native',type=Path)
args=parser.parse_args();positive=decode(args.original,'original');expected(positive)
assert decode(args.native,'native')==positive;passed=[]
for platform,directory in (('original',args.original),('native',args.native)):
    path=directory/'pass-state.bin';original=path.read_bytes()
    def negative(label,file,change):
        old=file.read_bytes()
        try:
            file.write_bytes(change(old))
            try:
                result=decode(directory,platform);expected(result)
                assert result==positive
            except (AssertionError,KeyError,ValueError,struct.error):passed.append(platform+': '+label)
            else:raise AssertionError('pass effect ignored: '+label)
        finally:file.write_bytes(old)
    def flip(position):return lambda b:b[:position]+bytes([b[position]^1])+b[position+1:]
    for i in range(5):
        negative('pass%d flags'%i,path,flip(12+36*i+16))
        negative('pass%d selector'%i,path,flip(12+36*i+12))
    negative('defined char-array tail',path,flip(12+14))
    negative('table record width',path,flip(8))
    negative('extra raw payload',path,lambda b:b+b'X')
    position=192;descriptors={}
    for i in range(5):
        for slot in (0,1,2,8):
            pointer,length=struct.unpack_from('<2I',original,position)
            descriptors[i,slot]=(position,position+8,length,pointer);position+=8+length
    offset,text,length,pointer=descriptors[1,8]
    assert original[text:text+length]==b'.pre\0'
    negative('assigned leading argument bytes',path,flip(text))
    negative('bounded pointed string length',path,flip(offset+4))
    def broken_alias(b):
        data=bytearray(b);entry=descriptors[2,2];new=entry[3]+4
        struct.pack_into('<I',data,12+36*2+8,new);struct.pack_into('<I',data,entry[0],new);return data
    negative('unchanged shared environment alias',path,broken_alias)
    negative('control binding',directory/'pass-state-observer.txt',lambda b:b+b'X')
    if platform=='original':
        negative('actual entry site',directory/'events.txt',lambda b:b.replace(b'site=004052a5 count=1',b'site=004052a6 count=1'))
        negative('one actual observation',directory/'events.txt',lambda b:b.replace(b'site=004052a5 count=1',b'site=004052a5 count=2'))
    else:
        def wrong(key,value):
            def change(b):
                item=json.loads(b);item[key]=value;return(json.dumps(item)+'\n').encode()
            return change
        negative('actual entry name',directory/'pass-state-gdb.json',wrong('symbol','other'))
        negative('one actual observation',directory/'pass-state-gdb.json',wrong('count',2))
    assert path.read_bytes()==original and decode(directory,platform)==positive
print(json.dumps({'result':'PASS','negatives':len(passed),'controls':passed}))
