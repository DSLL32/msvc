#!/usr/bin/env python3
"""Read-only PE inventory. This does not discover or accept reconstructed code."""
import hashlib
import json
from pathlib import Path
import struct


def inspect(path):
    data = path.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('not PE')
    machine, count, stamp, _, _, size, flags = struct.unpack_from('<HHIIIHH', data, pe + 4)
    opt = pe + 24
    if struct.unpack_from('<H', data, opt)[0] != 0x10b:
        raise ValueError('not PE32')
    base = struct.unpack_from('<I', data, opt + 28)[0]
    sections = []
    for index in range(count):
        offset = opt + size + index * 40
        name, virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<8sIIII', data, offset)
        sections.append(dict(name=name.rstrip(b'\0').decode('ascii'), rva=rva,
                             virtual_size=virtual_size, raw_size=raw_size, raw_offset=raw_offset,
                             characteristics=struct.unpack_from('<I', data, offset + 36)[0]))

    def file_offset(rva):
        for section in sections:
            delta = rva - section['rva']
            if 0 <= delta < section['raw_size']:
                return section['raw_offset'] + delta
        raise ValueError('RVA is not file backed: %x' % rva)

    def string(rva):
        offset = file_offset(rva)
        return data[offset:data.index(0, offset)].decode('ascii')

    directories = [struct.unpack_from('<II', data, opt + 96 + i * 8) for i in range(16)]
    imports = []
    offset = file_offset(directories[1][0])
    while any(data[offset:offset + 20]):
        lookup, _, _, name, address = struct.unpack_from('<IIIII', data, offset)
        item = dict(dll=string(name), entries=[])
        thunk = file_offset(lookup or address)
        while True:
            value = struct.unpack_from('<I', data, thunk)[0]
            if not value:
                break
            item['entries'].append('#%d' % (value & 0xffff) if value & 0x80000000 else string(value + 2))
            thunk += 4
        imports.append(item)
        offset += 20
    return dict(path=str(path), sha256=hashlib.sha256(data).hexdigest(), machine=machine,
                image_base=base, entry_rva=struct.unpack_from('<I', data, opt + 16)[0],
                coff_timestamp=stamp, characteristics=flags, sections=sections,
                tls_directory=dict(rva=directories[9][0], size=directories[9][1]),
                imports=imports, exports_directory=dict(rva=directories[0][0], size=directories[0][1]))


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[4]
    print(json.dumps(inspect(root / 'toolchains/5.10/bin/LINK.EXE'), indent=2))
