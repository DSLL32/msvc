"""Author a PE32/NB05 project with native wide C7 field-list continuations.

This is format-authored input, not historical compiler output. NB05 directory,
OMF module and signature-2 C7 layouts are those consumed by VC5 CVPACK. Every
index is regenerated as DWORD-wide; no old stream is signature-relabeled.
"""
import argparse
from pathlib import Path
import struct

H = lambda value: struct.pack('<H', value)
I = lambda value: struct.pack('<I', value)


def counted(name):
    raw = name.encode('ascii')
    assert len(raw) < 256
    return bytes([len(raw)]) + raw


def pad(raw):
    count = -len(raw) % 4
    return raw + bytes(range(0xf0 + count, 0xf0, -1))


def record(kind, payload):
    raw = pad(H(0) + H(kind) + payload)
    return H(len(raw) - 2) + raw[2:]


def types_and_symbols(module):
    types, symbols, families = [], [], []
    for family, chunks in [('Pair', [1, 1]), ('Samples', [6, 7, 7]),
                           ('Telemetry', [40, 40, 20])]:
        first = 0x1000 + len(types)
        names = []
        ordinal = 0
        for chunk_number, count in enumerate(chunks):
            fields = bytearray()
            for _ in range(count):
                name = chr(97 + ordinal) if family == 'Pair' else 'f%03d' % ordinal
                names.append(name)
                # LF_MEMBER: attributes/public, DWORD primitive T_LONG, numeric
                # byte offset and counted name. Padding is LF_PAD, not a member.
                fields += pad(H(0x1405) + H(3) + I(0x74) + H(ordinal * 4) + counted(name))
                ordinal += 1
            if chunk_number + 1 < len(chunks):
                fields += H(0x1404) + H(0) + I(first + chunk_number + 1)
            types.append(record(0x1203, fields))
        class_index = 0x1000 + len(types)
        # Distinct class names in module2 preserve the second compaction while
        # identical field-list payloads can intern against module1's lists.
        class_name = family + str(module)
        types.append(record(0x1005, H(sum(chunks)) + H(0) + I(first) + I(0) + I(0)
                            + H(sum(chunks) * 4) + counted(class_name)))
        symbols.append(record(0x1003, I(class_index) + counted(class_name)))
        families.append((first, class_index, chunks, names))
    return I(2) + b''.join(types), I(2) + b''.join(symbols), families


def codeview():
    data = bytearray(b'NB05' + I(0))
    entries = []
    for module in (1, 2):
        types, symbols, families = types_and_symbols(module)
        # Two real contributions in PE section1, disjoint and within512 bytes.
        info = H(0) + H(0) + H(1) + b'CV'
        info += H(1) + H(0) + I((module - 1) * 0x100) + I(0x100)
        info += counted('schema%d.obj' % module)
        for kind, raw in [(0x120, info), (0x121, types), (0x124, symbols)]:
            data += b'\0' * (-len(data) % 4)
            entries.append((kind, module, len(data), len(raw)))
            data += raw
    directory = len(data)
    struct.pack_into('<I', data, 4, directory)
    data += struct.pack('<HHIII', 16, 12, len(entries), 0, 0)
    data += b''.join(struct.pack('<HHII', *entry) for entry in entries)
    data += b'NB05' + I(len(data) + 8)
    validate_codeview(data)
    return bytes(data)


def validate_codeview(data):
    assert data[:4] == data[-8:-4] == b'NB05'
    assert struct.unpack_from('<I', data, len(data) - 4)[0] == len(data)
    directory, = struct.unpack_from('<I', data, 4)
    header, entry_size, count, following, flags = struct.unpack_from('<HHIII', data, directory)
    assert (header, entry_size, count, following, flags) == (16, 12, 6, 0, 0)
    assert directory + header + count * entry_size + 8 == len(data)
    for module in (1, 2):
        rows = [struct.unpack_from('<HHII', data, directory + 16 + 12 * n)
                for n in range((module - 1) * 3, module * 3)]
        assert [row[:2] for row in rows] == [(0x120, module), (0x121, module), (0x124, module)]
        for _, _, offset, size in rows:
            assert 8 <= offset < offset + size <= directory
        _, _, offset, size = rows[0]
        info = data[offset:offset + size]
        assert struct.unpack_from('<HHH2sHHII', info) == (0, 0, 1, b'CV', 1, 0, (module - 1) * 0x100, 0x100)
        assert 21 + info[20] == len(info)
        _, _, offset, size = rows[1]
        raw = data[offset:offset + size]
        assert raw[:4] == I(2)
        records, cursor = {}, 4
        while cursor < len(raw):
            length, kind = struct.unpack_from('<HH', raw, cursor)
            assert length >= 2 and (length + 2) % 4 == 0
            assert cursor + length + 2 <= len(raw)
            records[0x1000 + len(records)] = raw[cursor:cursor + length + 2]
            cursor += length + 2
        assert cursor == len(raw) and len(records) == 11
        for index, raw in records.items():
            kind, = struct.unpack_from('<H', raw, 2)
            if kind != 0x1005:
                assert kind == 0x1203
                continue
            count, props, field, derived, vshape, size = struct.unpack_from('<HHIIIH', raw, 4)
            assert props == derived == vshape == 0 and size == count * 4
            seen, members, offsets = set(), [], []
            while field:
                assert field in records and field not in seen
                seen.add(field)
                fields = records[field]
                assert fields[2:4] == H(0x1203)
                position, field = 4, 0
                while position < len(fields):
                    if fields[position] >= 0xf0:
                        padding = fields[position] & 15
                        assert fields[position:position + padding] == bytes(range(0xf0 + padding, 0xf0, -1))
                        position += padding
                        continue
                    tag, attr, target = struct.unpack_from('<HHI', fields, position)
                    if tag == 0x1404:
                        assert attr == 0 and position + 8 == len(fields)
                        field = target
                        position += 8
                    else:
                        assert tag == 0x1405 and attr == 3 and target == 0x74
                        displacement, = struct.unpack_from('<H', fields, position + 8)
                        length = fields[position + 10]
                        members.append(bytes(fields[position + 11:position + 11 + length]))
                        offsets.append(displacement)
                        position += 11 + length
                assert position == len(fields)
            assert len(members) == len(set(members)) == count
            assert offsets == list(range(0, count * 4, 4))
        _, _, offset, size = rows[2]
        symbols = data[offset:offset + size]
        assert symbols[:4] == I(2)
        cursor = 4
        while cursor < len(symbols):
            length, kind, index = struct.unpack_from('<HHI', symbols, cursor)
            assert kind == 0x1003 and records[index][2:4] == H(0x1005)
            assert symbols[cursor + 8:cursor + 9 + symbols[cursor + 8]] == records[index][22:23 + records[index][22]]
            cursor += length + 2
        assert cursor == len(symbols)


def image(cv=None):
    if cv is None:
        cv = codeview()
    debug_size = (28 + len(cv) + 511) & ~511
    data = bytearray(0x400 + debug_size)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 0x84, 0x14c, 2, 0, 0, 0, 224, 0x2102)
    optional = 0x98
    struct.pack_into('<HBBIIIIII', data, optional, 0x10b, 5, 10, 0x200, debug_size, 0, 0, 0x1000, 0x2000)
    struct.pack_into('<III', data, optional + 28, 0x10000000, 0x1000, 0x200)
    struct.pack_into('<HHHHHH', data, optional + 40, 4, 0, 0, 0, 4, 0)
    struct.pack_into('<III', data, optional + 56, 0x2000 + ((debug_size + 4095) & ~4095), 0x200, 0)
    struct.pack_into('<HHIIIIII', data, optional + 68, 3, 0, 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    struct.pack_into('<II', data, optional + 96 + 6 * 8, 0x2000, 28)
    section = optional + 224
    struct.pack_into('<8sIIIIIIHHI', data, section, b'.text', 0x200, 0x1000, 0x200, 0x200, 0, 0, 0, 0, 0x60000020)
    struct.pack_into('<8sIIIIIIHHI', data, section + 40, b'.debug', 28 + len(cv), 0x2000, debug_size, 0x400, 0, 0, 0, 0, 0x42000040)
    data[0x200] = 0xc3
    struct.pack_into('<IIHHIIII', data, 0x400, 0, 0, 0, 0, 2, len(cv), 0x201c, 0x41c)
    data[0x41c:0x41c + len(cv)] = cv
    assert len(data) % 512 == 0
    assert data[0x41c:0x41c + len(cv)] == cv
    return bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_bytes(image())
