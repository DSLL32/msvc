"""Format-authored NB05 containing C6 types and 16/32-bit local symbols.

NB05 is the VC5 input container, while its SST_TYPES payload remains the real
C6 marker/WORD-length/byte-leaf format. VC5's wide BPREL stores a
DWORD type index; wide DATA stores DWORD type first, then offset/segment.
The UDT control form also carries a DWORD type. These actual consumed-field
layouts differ from VC1's narrow-index counterparts. No C7 signature or producer output is
relabelled. The PE envelope is shared with the wide-fieldlist input recipe.
"""
import argparse
from pathlib import Path
import runpy
import struct

H = lambda n: struct.pack('<H', n)
I = lambda n: struct.pack('<I', n)


def counted(text):
    raw = text.encode('ascii')
    assert len(raw) < 128
    return bytes([len(raw)]) + raw


def typ(leaf, fields=b'', marker=1):
    return bytes([marker]) + H(len(fields) + 1) + bytes([leaf]) + fields


# VC5 converter407e20: flags byte, primitive-byte return, calling convention63,
# numeric count0; branch407efb supplies theffff empty argument-list sentinel.
# Unlike VC1, a marker83 reference would be a DWORD here. Scalar leaf8d mode0
# (407d50) consumes its numeric byte count and emits LF_ARRAY1003 with fixed
# T_CHAR10/T_USHORT21 element/index types and an empty name. Opaque COBOL
# leavesa6/a7 are excluded until their payload semantics are established.
TYPES = [
    ('procedure', typ(0x75, b'\0\x85\x63\0\x83' + I(0xffff))),
    ('nil', typ(0x80)),
    ('untranslated', typ(0x8e)),
    ('label', typ(0x72, b'\0\x73')),
    ('bitfield', typ(0x5c, b'\x08\x7d\0')),
    ('scalar_array', typ(0x8d, b'\0\x40')),
    ('array', typ(0x78, b'\x40\x83' + H(0x85) + b'\x83' + H(0x85) + b'\x82' + counted('bytes'))),
]


def symbol(kind, fields=b''):
    assert len(fields) + 1 < 256
    return bytes([len(fields) + 1, kind]) + fields


def symbols(wide):
    offset = I if wide else H
    type_index = I if wide else H
    flag = 0x80 if wide else 0
    rows = [symbol(0x11, H(1))]
    rows += [symbol(1 | flag, offset(0) + H(0x200) + H(0x100) + H(0)
                    + H(0x100) + H(0) + b'\0' + counted('schema_worker'))]
    rows += [symbol(0x0d, H(0x200 + n) + b'\x11' + counted(name))
             for n, (name, _) in enumerate(TYPES[1:], 1)]
    rows += [symbol(4 | flag, offset(4) + type_index(0x205) + counted('scalar')),
             symbol(5 | flag, (I(0x206) + I(0x120) + H(1) if wide else H(0x120) + H(1) + H(0x206)) + counted('window')),
             symbol(0x0b | flag, offset(0x30) + b'\0' + counted('again')),
             symbol(flag, offset(0x40) + H(0x20) + counted('scope')),
             symbol(4 | flag, offset(8) + type_index(0x204) + counted('bits')),
             symbol(2)]
    for name, value in [('small', b'\x01\x07'), ('large', b'\x85' + H(65534)),
                        ('wide', b'\x86' + I(0x12345678))]:
        rows += [symbol(0x0e, H(0x86 if name == 'wide' else 0x85) + value + counted(name))]
    rows += [symbol(0x12, I(0x206) + counted('Samples')), symbol(0x10, b'\0\0'), symbol(2)]
    return b''.join(rows)


def validate_types(raw):
    cursor = 0
    for _, expected in TYPES:
        assert raw[cursor:cursor + len(expected)] == expected
        marker, length, leaf = struct.unpack_from('<BHB', raw, cursor)
        assert marker == (0 if leaf in (0xa6, 0xa7) else 1)
        assert length >= 1 and cursor + length + 3 <= len(raw)
        assert leaf not in (0x7a, 0x79, 0x8c, 0x91, 0xa6, 0xa7)
        cursor += length + 3
    assert cursor == len(raw)


def validate_symbols(raw, wide):
    cursor, depth, referenced = 0, 0, set()
    while cursor < len(raw):
        length, kind = raw[cursor:cursor + 2]
        end = cursor + length + 1
        assert length >= 1 and end <= len(raw)
        base = kind & 0x7f
        width = 4 if kind & 0x80 else 2
        if base in (0, 1, 4, 5, 0xb):
            assert bool(kind & 0x80) == wide
            offset_position = cursor + (6 if base == 5 and wide else 2)
            value = int.from_bytes(raw[offset_position:offset_position + width], 'little')
            assert value < 0x200
        if base in (0, 1):
            depth += 1
        if base == 2:
            depth -= 1
        assert depth >= 0
        type_offset = {1: 2 + width, 4: 2 + width,
                       5: 2 if wide else 4 + width,
                       0xd: 2, 0xe: 2, 0x12: 2}.get(base)
        type_width = 4 if base == 0x12 or (wide and base in (4, 5)) else 2
        if type_offset is not None:
            index = int.from_bytes(raw[cursor + type_offset:cursor + type_offset + type_width], 'little')
            assert index < 0x200 or 0x200 <= index < 0x200 + len(TYPES)
            referenced.add(index)
        name_offset = {0: 2 + width + 2, 1: 13 + width,
                       4: 2 + width + type_width,
                       5: 12 if wide else 8,
                       0xb: 3 + width, 0xd: 5, 0x12: 6}.get(base)
        if base == 0xe:
            numeric = cursor + 4
            tag = raw[numeric]
            name_offset = 4 + ({0x85: 3, 0x86: 5}.get(tag, 1 + tag))
        if name_offset is not None:
            name = cursor + name_offset
            assert name < end and name + 1 + raw[name] == end
        if base == 0x11:
            assert length == 3 and raw[cursor + 2:cursor + 4] == H(1)
        if base == 2:
            assert length == 1
        cursor = end
    assert cursor == len(raw) and depth == 0
    assert referenced >= set(range(0x200, 0x200 + len(TYPES)))


def codeview():
    raw_types = b''.join(raw for _, raw in TYPES)
    validate_types(raw_types)
    data, entries = bytearray(b'NB05' + I(0)), []
    for module, wide in [(1, False), (2, True)]:
        locals = symbols(wide)
        validate_symbols(locals, wide)
        info = H(0) + H(0) + H(1) + b'CV'
        info += H(1) + H(0) + I(0) + I(0x200) + counted('legacy%d.obj' % module)
        for kind, raw in [(0x120, info), (0x121, raw_types), (0x124, locals)]:
            data += b'\0' * (-len(data) % 4)
            entries.append((kind, module, len(data), len(raw)))
            data += raw
    directory = len(data)
    struct.pack_into('<I', data, 4, directory)
    data += struct.pack('<HHIII', 16, 12, len(entries), 0, 0)
    data += b''.join(struct.pack('<HHII', *row) for row in entries)
    data += b'NB05' + I(len(data) + 8)
    assert len(data) == directory + 16 + 12 * len(entries) + 8
    for kind, module, offset, size in entries:
        assert offset % 4 == 0 and 8 <= offset < offset + size <= directory
        if kind == 0x120:
            record = data[offset:offset + size]
            assert struct.unpack_from('<HHH2sHHII', record) == (0, 0, 1, b'CV', 1, 0, 0, 0x200)
            assert len(record) == 21 + record[20]
    return bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    envelope = runpy.run_path(str(Path(__file__).with_name('make-fieldlist-input.py')))
    args.output.write_bytes(envelope['image'](codeview()))
