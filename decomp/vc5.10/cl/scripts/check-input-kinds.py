#!/usr/bin/env python3
"""Bounded semantic controls for the extension/pass tables; no child execution."""
import copy
from pathlib import Path
import struct
from rules import ELF
from input_kinds import decode,decode_passes,verify

project=Path(__file__).resolve().parents[1];root=project.parents[2]
assert verify(project,root)['records']==10
native=ELF(project/'build/native-trace/CL.EXE');start=native.symbols['input_kinds']
expected=decode(native,start)


def patch(address,payload):
    image=copy.copy(native);data=bytearray(image.data)
    section=next(s for s in image.sections if s[3]<=address< s[3]+s[5])
    offset=section[4]+address-section[3];data[offset:offset+len(payload)]=payload
    image.data=bytes(data);return image


def rejects(image,decoder=decode,address=start,baseline=expected):
    try:value=decoder(image,address)
    except (AssertionError,UnicodeError):return
    assert value!=baseline,'changed typed table was ignored'


rejects(patch(start+4,struct.pack('<i',1)))
extension,_,passes=struct.unpack('<3I',native.read(start,12))
rejects(patch(extension,b'x'))
rejects(patch(start+12+8,struct.pack('<I',passes)))
rejects(patch(passes,struct.pack('<i',4)))
rejects(patch(start+10*12+4,struct.pack('<i',0)))
table=native.symbols['compiler_passes'];baseline=decode_passes(native,table)
rejects(patch(table+16,struct.pack('<I',0)),decode_passes,table,baseline)
rejects(patch(table+20,struct.pack('<I',1)),decode_passes,table,baseline)
print('Input kinds and compiler-pass records match three images; seven semantic mutation controls pass')
