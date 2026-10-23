"""Structural checks for the captured large initialized-data DLL workflow."""
import re
import struct


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def image(data):
    assert data[:2] == b'MZ'
    pe = u32(data, 60)
    assert data[pe:pe+4] == b'PE\0\0'
    machine, count, stamp, syms, nsyms, optional, flags = struct.unpack_from('<HHIIIHH', data, pe+4)
    assert machine == 0x14c and count == 4 and optional == 224 and flags & 0x2000
    opt = pe+24
    assert struct.unpack_from('<H', data, opt)[0] == 0x10b
    sections = []
    for i in range(count):
        pos = opt+optional+i*40
        name = data[pos:pos+8].rstrip(b'\0').decode('ascii')
        virtual_size, rva, size, offset = struct.unpack_from('<4I', data, pos+8)
        assert offset+size <= len(data)
        sections.append(dict(name=name, virtual_size=virtual_size, rva=rva, bytes=size, offset=offset))
    def file_offset(rva, size):
        matches = [s['offset']+rva-s['rva'] for s in sections if s['rva'] <= rva and rva+size <= s['rva']+s['bytes']]
        assert len(matches) == 1
        return matches[0]
    fields = [dict(offset=pe+8, bytes=4, name='PE COFF TimeDateStamp')]
    debug_rva, debug_size = struct.unpack_from('<II', data, opt+96+6*8)
    assert debug_size == 56
    debug_offset = file_offset(debug_rva, debug_size)
    debug = []
    for pos in range(debug_offset, debug_offset+debug_size, 28):
        characteristics, timestamp, major, minor, kind, size, address, offset = struct.unpack_from('<IIHHIIII', data, pos)
        assert timestamp == stamp and offset+size <= len(data)
        fields.append(dict(offset=pos+4, bytes=4, name='IMAGE_DEBUG_DIRECTORY TimeDateStamp type %d' % kind))
        debug.append(dict(type=kind, bytes=size, offset=offset, rva=address))
    assert [d['type'] for d in debug] == [1, 6]
    assert debug[1]['bytes'] == 36
    initialized = next(s for s in sections if s['name'] == '.data')
    payload = initialized['virtual_size']-268
    assert payload in (3*1024*1024, 5*1024*1024)
    pointer_rva = 0x3000+payload+264
    fixups = [struct.unpack_from('<III', data, debug[1]['offset']+i) for i in range(0, 36, 12)]
    assert fixups == [(6, pointer_rva, 0x3000), (6, 0x1007, 0x3000), (6, 0x101c, 0x3000)]
    base, entry = u32(data, opt+28), u32(data, opt+16)
    assert base == 0x400000 and entry == 0x1000
    stack_heap = struct.unpack_from('<4I', data, opt+72)
    assert stack_heap == (0x200000, 0x3000, 0x100000, 0x2000)
    reloc_rva, reloc_size = struct.unpack_from('<II', data, opt+96+5*8)
    cursor = file_offset(reloc_rva, reloc_size)
    end, relocations = cursor+reloc_size, []
    while cursor < end:
        page, size = struct.unpack_from('<II', data, cursor)
        assert size >= 8 and not size % 2 and cursor+size <= end
        for pos in range(cursor+8, cursor+size, 2):
            word = struct.unpack_from('<H', data, pos)[0]
            if word >> 12:
                assert word >> 12 == 3
                relocations.append(page+(word & 4095))
        cursor += size
    assert relocations == [0x1007, 0x101c, pointer_rva]
    start = initialized['offset']
    assert data[start:start+3] == b'abc'
    assert not any(data[start+3:start+payload+257])
    assert u32(data, file_offset(pointer_rva-4, 4)) == 0x13579bdf
    assert u32(data, file_offset(pointer_rva, 4)) == 0x40307b
    assert len(data) in ((4194304, 3156992) if payload == 3*1024*1024 else (5259776,))
    return dict(bytes=len(data), sections=sections, timestamp=stamp, fields=fields,
                entry_rva=entry, image_base=base, stack_heap=stack_heap,
                highlow_relocations=relocations, debug=debug, fixups=fixups)


def timestamp_fields(data, kind):
    if kind == 'raw':
        return []
    if kind == 'pe-coff-fixup':
        return image(data)['fields']
    assert kind == 'map'
    matches = list(re.finditer(rb'^ Timestamp is ([0-9a-f]{8}) \(([^\r\n]{24})\)\r?$', data, re.M))
    assert len(matches) == 1
    return [dict(offset=matches[0].start(i), bytes=matches[0].end(i)-matches[0].start(i), name=name)
            for i, name in [(1, 'Map COFF timestamp hexadecimal'), (2, 'Map timestamp ctime rendering')]]


def normalized(data, fields):
    result = bytearray(data)
    for field in fields:
        start, size = field['offset'], field['bytes']
        assert 0 <= start <= start+size <= len(result)
        result[start:start+size] = bytes(size)
    return bytes(result)
