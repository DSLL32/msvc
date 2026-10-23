#!/usr/bin/env python3
"""Read-only PE inventory; no masking, graph discovery, or acceptance."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / 'toolchains/5.10/bin/CVTRES.EXE'

def inventory():
    data = TARGET.read_bytes()
    u16 = lambda o: struct.unpack_from('<H', data, o)[0]
    u32 = lambda o: struct.unpack_from('<I', data, o)[0]
    pe = u32(0x3c)
    assert data[pe:pe+4] == b'PE\0\0'
    opt = pe + 24
    assert u16(opt) == 0x10b
    sections = []
    for i in range(u16(pe + 6)):
        o = opt + u16(pe + 20) + i * 40
        sections.append(dict(name=data[o:o+8].rstrip(b'\0').decode('ascii'),
                             rva=u32(o+12), virtual_size=u32(o+8),
                             raw_offset=u32(o+20), raw_size=u32(o+16),
                             characteristics=u32(o+36)))
    def offset(rva):
        if rva < u32(opt+60):
            return rva
        for s in sections:
            if s['rva'] <= rva < s['rva'] + s['raw_size']:
                return s['raw_offset'] + rva - s['rva']
        raise ValueError('non-file-backed RVA %x' % rva)
    def string(rva):
        o = offset(rva)
        return data[o:data.index(0, o)].decode('ascii')
    directories = [(u32(opt+96+i*8), u32(opt+100+i*8))
                   for i in range(u32(opt+92))]
    imports = {}
    if directories[1][0]:
        o = offset(directories[1][0])
        while any(data[o:o+20]):
            lookup, _, _, name, iat = struct.unpack_from('<IIIII', data, o)
            names = []
            t = offset(lookup or iat)
            while u32(t):
                value = u32(t)
                names.append('#%d' % (value & 0xffff) if value & 0x80000000
                             else string(value+2))
                t += 4
            imports[string(name)] = names
            o += 20
    return dict(schema=1, target=str(TARGET.relative_to(ROOT)),
                sha256=hashlib.sha256(data).hexdigest(), size=len(data),
                machine=u16(pe+4), characteristics=u16(pe+22),
                image_base=u32(opt+28), entry_rva=u32(opt+16),
                linker_version=[data[opt+2], data[opt+3]],
                subsystem=u16(opt+68), sections=sections, imports=imports,
                tls_directory=dict(rva=directories[9][0],size=directories[9][1]),
                export_directory=dict(rva=directories[0][0],size=directories[0][1]),
                base_relocation_directory=dict(rva=directories[5][0],size=directories[5][1]))

if __name__ == '__main__':
    print(json.dumps(inventory(), indent=2))
