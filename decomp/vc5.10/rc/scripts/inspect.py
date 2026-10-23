#!/usr/bin/env python3
"""Read-only PE inventory. This is not graph discovery or acceptance."""
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / 'toolchains/5.10/bin/RC.EXE'

def main():
    data = TARGET.read_bytes()
    pe = struct.unpack_from('<I', data, 60)[0]
    machine, count = struct.unpack_from('<HH', data, pe + 4)
    optional = pe + 24
    size_optional = struct.unpack_from('<H', data, pe + 20)[0]
    sections = []
    for i in range(count):
        off = optional + size_optional + i * 40
        name = data[off:off + 8].rstrip(b'\0').decode('ascii')
        vsize, rva, rawsize, rawoff = struct.unpack_from('<IIII', data, off + 8)
        sections.append(dict(name=name, rva=hex(rva), virtual_bytes=vsize,
                             raw_bytes=rawsize, raw_offset=rawoff,
                             characteristics=hex(struct.unpack_from('<I', data, off + 36)[0])))
    dirs = []
    for i, name in [(0, 'exports'), (1, 'imports'), (2, 'resources'), (5, 'relocations'), (9, 'tls')]:
        rva, size = struct.unpack_from('<II', data, optional + 96 + 8 * i)
        dirs.append(dict(name=name, rva=hex(rva), size=size))
    result = dict(target='toolchains/5.10/bin/RC.EXE', bytes=len(data),
                  sha256=hashlib.sha256(data).hexdigest(), machine=hex(machine),
                  image_base=hex(struct.unpack_from('<I', data, optional + 28)[0]),
                  entry_rva=hex(struct.unpack_from('<I', data, optional + 16)[0]),
                  sections=sections, directories=dirs,
                  static_dependencies=['MSVCRT.dll', 'KERNEL32.dll', 'RCDLL.dll'],
                  rcdll_imports=['Handler', 'RC'], reconstruction_started=False)
    print(json.dumps(result, indent=2))
    return result

if __name__ == '__main__':
    main()
