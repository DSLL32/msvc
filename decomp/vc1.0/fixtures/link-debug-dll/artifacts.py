"""Locate clock fields structurally; validate and derive the PE checksum."""
import re
import struct


def u16(data, offset):
    return struct.unpack_from('<H', data, offset)[0]


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def sections(data, start, count):
    return [struct.unpack_from('<8sIIIIIIHHI', data, start + 40 * i) for i in range(count)]


def pe_layout(data):
    assert data[:2] == b'MZ'
    pe = u32(data, 60)
    assert data[pe:pe + 4] == b'PE\0\0'
    optional = pe + 24
    records = sections(data, optional + u16(data, pe + 20), u16(data, pe + 6))

    def raw(rva):
        for section in records:
            if section[2] <= rva < section[2] + max(section[1], section[3]):
                assert section[4] and rva - section[2] < section[3]
                return section[4] + rva - section[2]
        raise AssertionError('Unmapped RVA %x' % rva)

    return pe, optional, raw


def checksum(data):
    image = bytearray(data)
    _, optional, _ = pe_layout(image)
    image[optional + 64:optional + 68] = bytes(4)
    if len(image) & 1:
        image.append(0)
    total = 0
    for offset in range(0, len(image), 2):
        total += u16(image, offset)
        total = (total & 65535) + (total >> 16)
    total = (total & 65535) + (total >> 16)
    return total + len(data)


def timestamp_fields(data, kind):
    fields = []

    def add(offset, size, name):
        assert 0 <= offset and offset + size <= len(data)
        fields.append(dict(offset=offset, bytes=size, name=name))

    def coff(base, size):
        assert u16(data, base) == 0x14c
        add(base + 4, 4, 'Generated COFF TimeDateStamp')
        table = base + 20 + u16(data, base + 16)
        for section in sections(data, table, u16(data, base + 2)):
            if section[0].rstrip(b'\0') == b'.edata':
                assert section[3] >= 40
                add(base + section[4] + 4, 4, 'Export directory TimeDateStamp')

    if kind in ('archive-members', 'archive-generated'):
        assert data[:8] == b'!<arch>\n'
        cursor = 8
        while cursor < len(data) and any(data[cursor:]):
            assert data[cursor + 58:cursor + 60] == b'`\n'
            size = int(data[cursor + 48:cursor + 58].strip(b' \0'))
            add(cursor + 16, 12, 'Archive member date')
            start = cursor + 60
            if kind == 'archive-generated' and data[start:start + 2] == b'\x4c\x01':
                coff(start, size)
            cursor = start + size + (size & 1)
        assert cursor == len(data) or not any(data[cursor:])
    elif kind == 'coff':
        coff(0, len(data))
    elif kind == 'debug-pe':
        pe, optional, raw = pe_layout(data)
        add(pe + 8, 4, 'PE COFF TimeDateStamp')
        rva, size = struct.unpack_from('<II', data, optional + 96)
        assert rva and size
        add(raw(rva) + 4, 4, 'Export directory TimeDateStamp')
        rva, size = struct.unpack_from('<II', data, optional + 96 + 6 * 8)
        assert rva and size % 28 == 0
        types = []
        for offset in range(raw(rva), raw(rva) + size, 28):
            add(offset + 4, 4, 'Debug directory TimeDateStamp')
            types.append(u32(data, offset + 12))
        assert types == [1, 2, 3], types
        rva, size = struct.unpack_from('<II', data, optional + 96 + 2 * 8)
        root = raw(rva)
        seen = set()

        def resource_directory(relative):
            assert relative not in seen and relative + 16 <= size
            seen.add(relative)
            directory = root + relative
            add(directory + 4, 4, 'Resource directory TimeDateStamp')
            count = u16(data, directory + 12) + u16(data, directory + 14)
            assert relative + 16 + count * 8 <= size
            for i in range(count):
                target = u32(data, directory + 16 + i * 8 + 4)
                if target & 0x80000000:
                    resource_directory(target & 0x7fffffff)
        resource_directory(0)
    elif kind == 'map':
        matches = list(re.finditer(rb'^ Timestamp is ([0-9a-f]{8}) \(([^\r\n]{24})\)\r?$', data, re.M))
        assert len(matches) == 1
        for group, name in [(1, 'Map PE timestamp hexadecimal'), (2, 'Map timestamp ctime rendering')]:
            add(matches[0].start(group), matches[0].end(group) - matches[0].start(group), name)
    elif kind != 'raw':
        raise ValueError(kind)
    return sorted(fields, key=lambda field: field['offset'])


def normalize_artifact(data, kind):
    image = bytearray(data)
    if kind == 'debug-pe':
        _, optional, _ = pe_layout(image)
        assert u32(image, optional + 64) == checksum(image), 'PE checksum is incorrect'
    for field in timestamp_fields(image, kind):
        image[field['offset']:field['offset'] + field['bytes']] = bytes(field['bytes'])
    if kind == 'debug-pe':
        # The checksum remains checked, and changes solely with clock fields.
        struct.pack_into('<I', image, optional + 64, checksum(image))
    return bytes(image)
