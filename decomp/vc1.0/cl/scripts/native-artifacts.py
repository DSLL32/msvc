"""Identified COFF/PE/archive/map clock fields only; no program execution."""
import re
from binary import PE,u16,u32

def normalized_artifact(path):
    data = bytearray(path.read_bytes())
    ignored = []
    def zero(offset, size, reason):
        ignored.append(dict(offset=offset, size=size, reason=reason))
        data[offset:offset+size] = b'\0' * size
    def coff(base):
        zero(base+4, 4, 'COFF TimeDateStamp')
        for i in range(u16(data, base+2)):
            section = base+20+u16(data, base+16)+40*i
            if data[section:section+8].rstrip(b'\0') == b'.edata':
                zero(base+u32(data, section+20)+4, 4, 'export-directory TimeDateStamp')
    if data[:2] == b'MZ':
        pe = PE(path)
        header = u32(data, 0x3c)
        zero(header+8, 4, 'PE COFF TimeDateStamp')
        export = u32(data, pe.optional+96)
        if export: zero(pe.offset(pe.base+export)+4, 4, 'export-directory TimeDateStamp')
    elif data[:2] == b'\x4c\x01': coff(0)
    elif data[:8] == b'!<arch>\n':
        offset = 8
        while offset < len(data) and data[offset] != 0:
            assert data[offset+58:offset+60] == b'`\n'
            size = int(data[offset+48:offset+58].rstrip(b'\0 '))
            zero(offset+16, 12, 'archive-member decimal modification timestamp')
            if data[offset+60:offset+62] == b'\x4c\x01': coff(offset+60)
            offset += 60+size+(size&1)
    elif path.suffix.lower() == '.map':
        match = re.search(rb'(?m)^ Timestamp is [^\r\n]+', data)
        assert match, 'expected original LINK map timestamp line'
        ignored.append(dict(offset=match.start(), size=match.end()-match.start(), reason='map timestamp line'))
        data[match.start():match.end()] = b' Timestamp is <timestamp>'
    return bytes(data), ignored
