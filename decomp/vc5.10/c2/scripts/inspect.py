#!/usr/bin/env python3
"""Read-only PE inventory; no discovery, masking or function acceptance."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
TARGET = ROOT / 'toolchains/5.10/bin/C2.EXE'

def inventory():
    data = TARGET.read_bytes()
    u16 = lambda pos: struct.unpack_from('<H', data, pos)[0]
    u32 = lambda pos: struct.unpack_from('<I', data, pos)[0]
    pe = u32(0x3c)
    assert data[pe:pe + 4] == b'PE\0\0'
    coff = pe + 4
    optional = coff + 20
    assert u16(optional) == 0x10b
    sections = []
    for i in range(u16(coff + 2)):
        p = optional + u16(coff + 16) + i * 40
        sections.append(dict(name=data[p:p+8].split(b'\0')[0].decode(),
                             virtual_size=u32(p+8), rva=u32(p+12),
                             raw_size=u32(p+16), raw_offset=u32(p+20),
                             characteristics=hex(u32(p+36))))
    def offset(rva):
        if rva < u32(optional + 60):
            return rva
        for s in sections:
            delta = rva - s['rva']
            if 0 <= delta < s['raw_size']:
                return s['raw_offset'] + delta
        raise ValueError('RVA is not file backed: %x' % rva)
    def string(rva):
        p = offset(rva)
        return data[p:data.index(b'\0', p)].decode('ascii')
    directories = {}
    names = ('export', 'import', 'resource', 'exception', 'security',
             'relocation', 'debug', 'architecture', 'global_ptr', 'tls',
             'load_config', 'bound_import', 'iat', 'delay_import', 'clr', 'reserved')
    for i, name in enumerate(names):
        directories[name] = dict(rva=u32(optional+96+8*i), size=u32(optional+100+8*i))
    imports = {}
    p = offset(directories['import']['rva'])
    while any(data[p:p+20]):
        thunk, _, _, dll, first = struct.unpack_from('<IIIII', data, p)
        q = offset(thunk or first)
        entries = []
        while u32(q):
            v = u32(q)
            entries.append(dict(ordinal=v & 65535) if v & 0x80000000 else
                           dict(name=string(v+2), hint=u16(offset(v))))
            q += 4
        imports[string(dll)] = entries
        p += 20
    base = u32(optional + 28)
    return dict(target=str(TARGET.relative_to(ROOT)), sha256=hashlib.sha256(data).hexdigest(),
                size=len(data), machine=hex(u16(coff)), characteristics=hex(u16(coff+18)),
                image_base=hex(base), entry_rva=hex(u32(optional+16)),
                entry_va=hex(base+u32(optional+16)), subsystem=u16(optional+68),
                image_size=u32(optional+56), sections=sections,
                directories=directories, imports=imports,
                no_function_reconstruction=True)

if __name__ == '__main__':
    print(json.dumps(inventory(), indent=2))
