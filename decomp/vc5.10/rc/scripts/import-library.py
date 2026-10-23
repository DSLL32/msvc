#!/usr/bin/env python3
"""Build stdcall link names with the original LIB, retain undecorated DLL lookup.

LIB 5 decorates COFF symbols for decorated DEF export names but also retains that
suffix in the DLL hint/name record. Remove only that suffix, keeping member sizes,
relocations and indexes unchanged. This generates import metadata, not target code.
"""
from pathlib import Path
import subprocess
from binary import COFF,u16,u32

out=Path('build/rcdll.lib')
subprocess.run(['python3','scripts/wine-tool.py','LIB.EXE','/nologo','/def:src/rcdll.def','/machine:IX86','/out:'+str(out)],check=True)
data=bytearray(out.read_bytes());offset=8;patched=[]
while offset<len(data):
    head=data[offset:offset+60]
    if not head.strip(b'\0'):break
    size=int(head[48:58].strip(b'\0 '));begin=offset+60
    member=bytes(data[begin:begin+size])
    if member[:2]==b'L\1':
        coff=COFF(member)
        for n,section in enumerate(coff.sections):
            if section['name']!='.idata$6':continue
            hint=section['data'];name=hint[2:].split(b'\0',1)[0]
            if name not in [b'Handler@4',b'RC@24']:continue
            replacement=hint[:2]+name.split(b'@')[0]+b'\0'
            raw=u32(member,20+u16(member,16)+40*n+20)
            data[begin+raw:begin+raw+len(hint)]=replacement.ljust(len(hint),b'\0')
            patched.append(name.decode())
    offset=begin+size+(size&1)
assert sorted(patched)==['Handler@4','RC@24'],'Unexpected generated import names'
out.write_bytes(data)
