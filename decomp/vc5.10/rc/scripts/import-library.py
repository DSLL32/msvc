#!/usr/bin/env python3
"""Generate RC's two stdcall import names, retaining undecorated DLL lookups.

Only LIB-created .idata$6 spelling is changed; indexes, relocations, sizes and
all other bytes stay unchanged. This is target-specific import-library data.
"""
from pathlib import Path
import struct
import subprocess
import sys

out = Path(sys.argv[1])
subprocess.run([sys.argv[2], 'original', 'tool', '5.10', 'LIB.EXE', '--',
                '/nologo', '/def:scripts/rcdll.def', '/machine:IX86', '/out:' + str(out)], check=True)
data = bytearray(out.read_bytes())
assert data[:8] == b'!<arch>\n'
offset = 8
patched = []
while offset < len(data):
    header = data[offset:offset + 60]
    if not header.strip(b'\0'):
        break
    size = int(header[48:58].strip(b'\0 '))
    begin = offset + 60
    if data[begin:begin + 2] == b'L\1':
        section_count = struct.unpack_from('<H', data, begin + 2)[0]
        optional_size = struct.unpack_from('<H', data, begin + 16)[0]
        for n in range(section_count):
            section = begin + 20 + optional_size + n * 40
            if data[section:section + 8] != b'.idata$6':
                continue
            length, raw = struct.unpack_from('<II', data, section + 16)
            raw += begin
            hint = data[raw:raw + length]
            name = hint[2:].split(b'\0', 1)[0]
            if name not in [b'Handler@4', b'RC@24']:
                continue
            value = hint[:2] + name.split(b'@')[0] + b'\0'
            data[raw:raw + length] = value.ljust(length, b'\0')
            patched.append(name.decode())
    offset = begin + size + (size & 1)
assert sorted(patched) == ['Handler@4', 'RC@24']
out.write_bytes(data)
