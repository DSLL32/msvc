#!/usr/bin/env python3
"""Read-only CL5.10 PE inventory; no graph discovery or acceptance."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / 'toolchains/5.10/bin/CL.EXE'


def inventory():
    data = TARGET.read_bytes()
    pe, = struct.unpack_from('<I', data, 0x3c)
    assert data[pe:pe + 4] == b'PE\0\0'
    machine, count, timestamp, _, _, optional_size, flags = struct.unpack_from('<HHIIIHH', data, pe + 4)
    optional = pe + 24
    assert struct.unpack_from('<H', data, optional)[0] == 0x10b
    entry, = struct.unpack_from('<I', data, optional + 16)
    base, = struct.unpack_from('<I', data, optional + 28)
    sections = []
    for index in range(count):
        at = optional + optional_size + index * 40
        name = data[at:at + 8].rstrip(b'\0').decode('ascii')
        size, rva, raw_size, raw_at = struct.unpack_from('<IIII', data, at + 8)
        characteristics, = struct.unpack_from('<I', data, at + 36)
        sections.append(dict(name=name, rva=rva, virtual_size=size,
                             raw_offset=raw_at, raw_size=raw_size,
                             characteristics=characteristics))

    def offset(rva):
        for section in sections:
            relative = rva - section['rva']
            if 0 <= relative < section['raw_size']:
                return section['raw_offset'] + relative
        raise ValueError('RVA outside file-backed sections: %#x' % rva)

    def string(rva):
        at = offset(rva)
        return data[at:data.index(b'\0', at)].decode('ascii')

    imports = {}
    directory, directory_size = struct.unpack_from('<II', data, optional + 104)
    descriptor = offset(directory)
    while True:
        lookup, _, _, name_rva, _ = struct.unpack_from('<IIIII', data, descriptor)
        if not lookup:
            break
        symbols = []
        at = offset(lookup)
        while True:
            value, = struct.unpack_from('<I', data, at)
            if not value:
                break
            symbols.append({'ordinal': value & 0xffff} if value & 0x80000000 else {'name': string(value + 2)})
            at += 4
        imports[string(name_rva)] = symbols
        descriptor += 20
    directories = {}
    for index, name in [(0, 'export'), (5, 'relocation'), (9, 'tls'), (13, 'delay_import')]:
        rva, size = struct.unpack_from('<II', data, optional + 96 + index * 8)
        directories[name] = dict(rva=rva, size=size)
    return dict(schema=1, status='inventory-only; reconstruction not started',
                target=str(TARGET.relative_to(ROOT)), sha256=hashlib.sha256(data).hexdigest(),
                bytes=len(data), machine=machine, timestamp=timestamp,
                image_base=base, entry_rva=entry, coff_characteristics=flags,
                sections=sections, imports=imports, directories=directories)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Verify saved inventory without changing it')
    args = parser.parse_args()
    record = inventory()
    expected = Path('cases/inventory.json')
    if args.check:
        assert json.loads(expected.read_text()) == record, 'PE inventory changed'
        print('CL5.10 inventory verified')
    else:
        print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
