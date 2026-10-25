#!/usr/bin/env python3
"""Fix the two missing NT import hint/name records emitted by original LIB.

Only .idata$6 payload strings change; COFF symbols, hints and offsets stay intact.
This is a target-specific import-library correction, not a general PE indexer.
"""
from pathlib import Path
import struct
import sys
path=Path(sys.argv[1]);data=bytearray(path.read_bytes())
assert data[:8]==b'!<arch>\n'
changed=[];offset=8
while offset<len(data):
    if not data[offset:offset+60].strip(b'\0'):break
    size=int(data[offset+48:offset+58].strip(b'\0 '));begin=offset+60
    if data[begin:begin+2]==b'L\x01':
        sections=struct.unpack_from('<H',data,begin+2)[0]
        optional=struct.unpack_from('<H',data,begin+16)[0]
        positions=[begin+20+optional+40*i for i in range(sections)]
        if not any(data[p:p+8]==b'.text\0\0\0' for p in positions):
            offset=begin+size+(size&1);continue
        for section in positions:
            if data[section:section+8]!=b'.idata$6':continue
            length,raw=struct.unpack_from('<II',data,section+16)
            record=begin+raw
            decorated=bytes(data[record+2:record+length]).split(b'\0',1)[0]
            assert decorated in (b'RtlAnsiStringToUnicodeString@12',b'RtlUnicodeStringToAnsiString@12')
            name=decorated[:-3];replacement=name+b'\0'
            data[record+2:record+length]=replacement.ljust(length-2,b'\0')
            changed.append(name)
    offset=begin+size+(size&1)
assert sorted(changed)==[b'RtlAnsiStringToUnicodeString',b'RtlUnicodeStringToAnsiString']
path.write_bytes(data)
