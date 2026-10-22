"""Locate documented timestamp fields in this linker's COFF products."""
import struct


def sections(data, table, count):
    result = []
    for i in range(count):
        offset = table + 40 * i
        name = data[offset:offset + 8].rstrip(b'\0')
        virtual_size, address, size, raw = struct.unpack_from('<IIII', data, offset + 8)
        result.append((name, address, max(size, virtual_size), raw, size))
    return result


def pe_info(data):
    assert data[:2] == b'MZ'
    header = struct.unpack_from('<I', data, 60)[0]
    assert data[header:header + 4] == b'PE\0\0'
    count = struct.unpack_from('<H', data, header + 6)[0]
    optional = header + 24
    assert struct.unpack_from('<H', data, optional)[0] == 0x10b
    table = optional + struct.unpack_from('<H', data, header + 20)[0]
    records = sections(data, table, count)

    def file_offset(rva):
        for name, address, span, raw, size in records:
            if address <= rva < address + span:
                assert raw and rva - address < size
                return raw + rva - address
        raise AssertionError('unmapped RVA %x' % rva)

    export_rva, export_size = struct.unpack_from('<II', data, optional + 96)
    export = file_offset(export_rva)
    values = struct.unpack_from('<IIHHIIIIIII', data, export)
    functions = file_offset(values[8])
    names = file_offset(values[9])
    ordinals = file_offset(values[10])
    exports = []
    for i in range(values[7]):
        name = file_offset(struct.unpack_from('<I', data, names + i * 4)[0])
        ordinal_index = struct.unpack_from('<H', data, ordinals + i * 2)[0]
        function_rva = struct.unpack_from('<I', data, functions + ordinal_index * 4)[0]
        exports.append(dict(name=data[name:data.index(0, name)].decode('ascii'),
                            ordinal=values[5] + ordinal_index, rva=function_rva))
    return dict(coff=header + 4, export=export, exports=exports,
                entry_rva=struct.unpack_from('<I', data, optional + 16)[0],
                dll=bool(struct.unpack_from('<H', data, header + 22)[0] & 0x2000))


def timestamp_fields(data, kind):
    fields = []

    def add(offset, size, name):
        fields.append(dict(offset=offset, bytes=size, name=name))

    def coff(base, size, export_object=False):
        assert struct.unpack_from('<H', data, base)[0] == 0x14c
        add(base + 4, 4, 'COFF TimeDateStamp')
        count = struct.unpack_from('<H', data, base + 2)[0]
        table = base + 20 + struct.unpack_from('<H', data, base + 16)[0]
        assert table + count * 40 <= base + size
        if export_object:
            for name, address, span, raw, raw_size in sections(data, table, count):
                if name == b'.edata':
                    assert raw_size >= 40
                    add(base + raw + 4, 4, 'Export directory TimeDateStamp')

    if kind == 'pe':
        info = pe_info(data)
        add(info['coff'] + 4, 4, 'PE COFF TimeDateStamp')
        add(info['export'] + 4, 4, 'Export directory TimeDateStamp')
    elif kind == 'coff':
        coff(0, len(data), export_object=True)
    elif kind == 'archive':
        assert data[:8] == b'!<arch>\n'
        cursor = 8
        while cursor < len(data):
            if not any(data[cursor:]):
                break  # Original linker leaves zero-filled preallocation.
            assert data[cursor + 58:cursor + 60] == b'`\n'
            name = data[cursor:cursor + 16].rstrip()
            size = int(data[cursor + 48:cursor + 58].strip(b' \0'))
            add(cursor + 16, 12, 'Archive member date')
            start = cursor + 60
            if data[start:start + 2] == b'\x4c\x01':
                coff(start, size, export_object=True)
            cursor = start + size + size % 2
        assert cursor == len(data) or not any(data[cursor:])
    else:
        raise ValueError(kind)
    return sorted(fields, key=lambda item: item['offset'])


def normalized(data, fields):
    result = bytearray(data)
    for field in fields:
        start, size = field['offset'], field['bytes']
        assert start >= 0 and start + size <= len(result)
        result[start:start + size] = bytes(size)
    return bytes(result)
