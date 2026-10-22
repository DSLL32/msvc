#!/usr/bin/env python3
"""Generate the missing NT import library with the original librarian.

LIB 2.42 preserves stdcall decoration in both the COFF symbol and DLL lookup
name. NT exports undecorated names. Keep decorated link symbols and replace
only each generated hint/name record, without changing archive sizes, code,
relocations or member indexes. No bytes are taken from the target executable.
"""
from pathlib import Path
import subprocess
from binary import COFF, u16, u32
import importlib.machinery
wt = importlib.machinery.SourceFileLoader('wine_tool', 'wine-tool.py').load_module()
output = Path('build/ntdll.lib')
subprocess.run(['python3', 'wine-tool.py', 'LIB.EXE', '/nologo',
                '/def:ntdll.def', '/out:' + str(output), '/machine:IX86'], check=True)
data = bytearray(output.read_bytes())
offset, changed = 8, []
while offset < len(data):
    if not data[offset:offset+60].strip(b'\0'):
        assert not data[offset:].strip(b'\0'), 'nonzero archive trailer'
        break
    size = int(data[offset+48:offset+58].strip(b'\0 '))
    begin = offset + 60
    member = bytes(data[begin:begin+size])
    if member[:2] == b'L\x01':
        obj = COFF(member)
        if not any(section['name'] == '.text' for section in obj.sections):
            offset = begin + size + (size & 1)
            continue
        for number, section in enumerate(obj.sections):
            if section['name'] != '.idata$6':
                continue
            hint_name = section['data']
            decorated = hint_name[2:].split(b'\0', 1)[0]
            assert decorated.endswith(b'@12'), decorated
            name = decorated[:-3]
            header = 20 + u16(member, 16) + number * 40
            raw = u32(member, header+20)
            replacement = hint_name[:2] + name + b'\0'
            assert len(replacement) <= len(hint_name)
            data[begin+raw:begin+raw+len(hint_name)] = replacement.ljust(len(hint_name), b'\0')
            changed.append(name.decode())
    offset = begin + size + (size & 1)
assert sorted(changed) == ['RtlAnsiStringToUnicodeString', 'RtlUnicodeStringToAnsiString']
output.write_bytes(data)
