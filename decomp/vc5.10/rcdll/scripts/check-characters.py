#!/usr/bin/env python3
"""Verify the two identified lexical tables, not surrounding unknown data."""
import hashlib
import json
import re
import struct
from pathlib import Path
HERE=Path(__file__).resolve().parents[1];ROOT=HERE.parents[2]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def pe(path):
    data=path.read_bytes();at=struct.unpack_from('<I',data,60)[0];optional=at+24;size=struct.unpack_from('<H',data,at+20)[0]
    base=struct.unpack_from('<I',data,optional+28)[0]
    sections=[struct.unpack_from('<IIII',data,optional+size+40*i+8)for i in range(struct.unpack_from('<H',data,at+6)[0])]
    def read(rva,length):
        for _,start,size,raw in sections:
            if start<=rva and rva+length<=start+size:return data[raw+rva-start:raw+rva-start+length]
        raise AssertionError('table is not fully PE-backed')
    return base,read

def verify():
    original=ROOT/'toolchains/5.10/bin/RCDLL.DLL';rebuilt=HERE/'build/RCDLL.DLL';mapfile=HERE/'build/rcdll.map';mapping=mapfile.read_text()
    _,old=pe(original);base,new=pe(rebuilt)
    def address(name):
        matches=re.findall(r'^\s*[0-9a-f]+:[0-9a-f]+\s+'+re.escape(name)+r'\s+([0-9a-f]+)\s',mapping,re.M|re.I);assert len(matches)==1;return int(matches[0],16)-base
    table=old(0x14010,512);assert new(address('_rc_character_tokens'),512)==table
    classes=old(0x14210,512);assert new(address('_rc_character_classes'),512)==classes
    alias=classes[:2];assert alias==b'\x80\0'
    return dict(status='pass',original_sha256=sha(original),rebuilt_sha256=sha(rebuilt),map_sha256=sha(mapfile),source_sha256=sha(HERE/'src/characters.c'),token_table=dict(original_rva='0x14010',rebuilt_rva=hex(address('_rc_character_tokens')),elements=256,element_bytes=2,bytes=512,sha256=hashlib.sha256(table).hexdigest()),class_table=dict(original_rva='0x14210',rebuilt_rva=hex(address('_rc_character_classes')),elements=256,element_bytes=2,bytes=512,sha256=hashlib.sha256(classes).hexdigest()),adjacent_alias=dict(original_rva='0x14210',class_index=0,value=128,bytes=2),scope='Only the identified256-WORD token and class tables; class index256/following keyword remains unclaimed. Runtime setup changes dollar class to16. No source/code credit for data.')
if __name__=='__main__':print(json.dumps(verify(),indent=2))
