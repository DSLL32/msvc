"""Minimal PE/COFF reader for recording reconstruction evidence."""
import struct
from pathlib import Path

def u16(data, offset): return struct.unpack_from('<H', data, offset)[0]
def u32(data, offset): return struct.unpack_from('<I', data, offset)[0]

class PE:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        pe = u32(self.data, 60)
        self.optional = pe + 24
        self.base = u32(self.data, self.optional + 28)
        self.entry = u32(self.data, self.optional + 16)
        self.sections = []
        start = self.optional + u16(self.data, pe + 20)
        for i in range(u16(self.data, pe + 6)):
            p = start + i * 40
            name = self.data[p:p+8].rstrip(b'\0').decode()
            vs, rva, size, off = struct.unpack_from('<IIII', self.data, p+8)
            self.sections.append(dict(name=name, rva=rva, size=size, off=off,
                                      virtual_size=vs, flags=u32(self.data, p+36)))

    def offset(self, va):
        rva = va - self.base
        for s in self.sections:
            if s['rva'] <= rva < s['rva'] + s['size']:
                return s['off'] + rva - s['rva']
        raise ValueError('unbacked address %08x' % va)

    def read(self, va, size):
        assert size>=0
        for section in self.sections:
            start=self.base+section['rva'];mapped=max(section['virtual_size'],section['size'])
            if start<=va and va+size<=start+mapped:
                relative=va-start;backed=max(0,min(size,section['size']-relative))
                offset=section['off']+relative
                return self.data[offset:offset+backed]+bytes(size-backed)
        raise ValueError('unmapped address range %08x + %x'%(va,size))

    def string(self, va):
        data=bytearray()
        for i in range(32768):
            byte=self.read(va+i,1)
            if byte==b'\0':return data.decode('latin1')
            data+=byte
        raise ValueError('unterminated PE string %08x'%va)

def archive(path):
    data = Path(path).read_bytes()
    assert data[:8] == b'!<arch>\n'
    p, names = 8, b''
    while p < len(data):
        header = data[p:p+60]
        size = int(header[48:58].rstrip(b'\0 '))
        name = header[:16].decode().strip()
        payload = data[p+60:p+60+size]
        if name == '//': names = payload
        elif name.startswith('/') and name[1:].isdigit():
            q = int(name[1:])
            end = names.find(b'\0', q)
            if end < 0: end = names.find(b'\n', q)
            yield names[q:end].decode().rstrip('/'), payload
        elif name not in ('/', '//'):
            yield name.rstrip('/'), payload
        p += 60 + size + (size & 1)

class COFF:
    def __init__(self, data):
        self.data = data
        assert u16(data, 0) == 0x14c
        symoff, count = u32(data, 8), u32(data, 12)
        strings = symoff + 18 * count
        self.symbols = {}
        i = 0
        while i < count:
            p = symoff + 18*i
            if data[p:p+4] == b'\0'*4:
                q = strings + u32(data, p+4)
                name = data[q:data.index(b'\0', q)].decode('latin1')
            else: name = data[p:p+8].rstrip(b'\0').decode('latin1')
            self.symbols[i] = dict(name=name, value=u32(data, p+8),
                                   section=struct.unpack_from('<h', data, p+12)[0],
                                   type=u16(data, p+14), storage=data[p+16])
            i += 1 + data[p+17]
        self.sections = []
        for i in range(u16(data, 2)):
            p = 20 + u16(data, 16) + 40*i
            size, off, reloc = struct.unpack_from('<III', data, p+16)
            relocs = []
            for j in range(u16(data, p+32)):
                q = reloc + 10*j
                relocs.append((u32(data, q), self.symbols[u32(data, q+4)], u16(data, q+8)))
            self.sections.append(dict(name=data[p:p+8].rstrip(b'\0').decode(),
                                      data=data[off:off+size] if off else b'',
                                      relocs=relocs, index=i+1, flags=u32(data,p+36)))

def match_section(haystack, section):
    needle = section['data']
    if not needle: return []
    mask = bytearray(b'\1' * len(needle))
    for off, sym, kind in section['relocs']:
        if kind in (6, 20): mask[off:off+4] = b'\0'*4
        else: return []
    runs = []
    p = 0
    while p < len(mask):
        if not mask[p]: p += 1; continue
        end = mask.find(0, p)
        if end < 0: end = len(mask)
        runs.append((p, end))
        p = end
    if not runs: return []
    a, b = max(runs, key=lambda r:r[1]-r[0])
    if b-a < 5: return []
    matches, p = [], 0
    while True:
        q = haystack.find(needle[a:b], p)
        if q < 0: break
        start = q-a
        if start >= 0 and all(haystack[start+x:start+y] == needle[x:y] for x,y in runs):
            matches.append(start)
        p = q+1
    return matches
