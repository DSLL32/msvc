"""Author NB05 C6 aggregate/argument-list schemas for the ordinary VC5 CLI.

OLF_LIST7f/STRUCTURE79/BARRAY8c, INDEX83 and STRING82 are the original C6
format, documented by Microsoft's microsoft-pdb/cvdump/{type6.cpp,typeinfo.h}.
VC5's procedure converter consumes a DWORD argument-list reference; its list
and structure readers consume marker83+WORD references. Those widths are
independently matched to the original target, not copied from VC1 output.
"""
import argparse
from pathlib import Path
import runpy
import struct

H = lambda value: struct.pack('<H', value)
I = lambda value: struct.pack('<I', value)


def counted(name):
    value = name.encode('ascii')
    assert len(value) < 128
    return bytes([len(value)]) + value


def typ(leaf, fields):
    return b'\1' + H(len(fields) + 1) + bytes([leaf]) + fields


def ref(index):
    assert index < 0x8000
    return b'\x83' + H(index)


def sym(kind, fields=b''):
    assert len(fields) < 254
    return bytes([len(fields) + 1, kind]) + fields


def schema(module):
    # Record indices200..205: callable, member types, member names/offsets,
    # aggregate, arguments and based array. Lists are owned through references.
    rows = [typ(0x75, b'\0\x85\x63\2\x83' + I(0x204)),
            typ(0x7f, ref(0x85) + ref(0x86)),
            typ(0x7f, b'\x82' + counted('code') + b'\0'
                + b'\x82' + counted('value') + b'\2'),
            typ(0x79, b'\x30\2' + ref(0x201) + ref(0x202)
                + b'\x82' + counted('Packet%d' % module) + b'\x69'),
            typ(0x7f, ref(0x203) + ref(0x205)),
            typ(0x8c, ref(0x203))]
    raw = b''.join(rows)
    validate_types(raw, module)
    symbols = sym(0x11, H(1))
    symbols += sym(1, H((module - 1) * 0x100) + H(0x200) + H(0x80)
                   + H(0) + H(0x80) + H(0) + b'\0' + counted('consume_packet'))
    symbols += sym(0x0d, H(0x203) + b'\x11' + counted('packet'))
    symbols += sym(0x0d, H(0x205) + b'\x12' + counted('elements'))
    symbols += sym(2)
    cursor, depth = 0, 0
    while cursor < len(symbols):
        length, kind = symbols[cursor:cursor + 2]
        end = cursor + length + 1
        assert end <= len(symbols)
        if kind == 1:
            offset, index, size, first, last, reserved = struct.unpack_from('<6H', symbols, cursor + 2)
            assert offset == (module - 1) * 0x100 and size == last == 0x80
            assert first == reserved == 0 and index == 0x200
            depth += 1
            name = cursor + 15
        elif kind == 0x0d:
            assert depth == 1 and struct.unpack_from('<H', symbols, cursor + 2)[0] in (0x203, 0x205)
            name = cursor + 5
        elif kind == 2:
            depth -= 1
            assert length == 1 and depth == 0
            cursor = end
            continue
        else:
            assert kind == 0x11 and length == 3 and symbols[cursor + 2:end] == H(1)
            cursor = end
            continue
        assert name + 1 + symbols[name] == end
        cursor = end
    assert cursor == len(symbols) and depth == 0
    return raw, symbols


def validate_types(raw, module):
    rows, cursor = [], 0
    while cursor < len(raw):
        marker, length, leaf = struct.unpack_from('<BHB', raw, cursor)
        end = cursor + length + 3
        assert marker == 1 and end <= len(raw) and length >= 1
        rows.append((leaf, raw[cursor + 4:end]))
        cursor = end
    assert cursor == len(raw) and [leaf for leaf, _ in rows] == [0x75, 0x7f, 0x7f, 0x79, 0x7f, 0x8c]
    assert rows[0][1] == b'\0\x85\x63\2\x83' + I(0x204)
    assert rows[1][1] == ref(0x85) + ref(0x86)
    assert rows[4][1] == ref(0x203) + ref(0x205)
    assert rows[5][1] == ref(0x203)
    fields = rows[3][1]
    assert fields[:8] == b'\x30\2' + ref(0x201) + ref(0x202)
    assert fields[8:] == b'\x82' + counted('Packet%d' % module) + b'\x69'
    names, cursor, offsets = rows[2][1], 0, []
    for name in ('code', 'value'):
        assert names[cursor] == 0x82
        cursor += 1
        assert names[cursor:cursor + 1 + names[cursor]] == counted(name)
        cursor += 1 + names[cursor]
        offsets.append(names[cursor])
        cursor += 1
    assert cursor == len(names) and offsets == [0, 2]
    # Primitive85 is2 bytes and86 is4 bytes; both fit the6-byte aggregate.
    assert all(offset + size <= 6 for offset, size in zip(offsets, [2, 4]))


def codeview():
    data, entries = bytearray(b'NB05' + I(0)), []
    for module in (1, 2):
        types, symbols = schema(module)
        info = H(0) + H(0) + H(1) + b'CV'
        info += H(1) + H(0) + I((module - 1) * 0x100) + I(0x100)
        info += counted('compound%d.obj' % module)
        for kind, raw in [(0x120, info), (0x121, types), (0x124, symbols)]:
            data += b'\0' * (-len(data) % 4)
            entries.append((kind, module, len(data), len(raw)))
            data += raw
    directory = len(data)
    struct.pack_into('<I', data, 4, directory)
    data += struct.pack('<HHIII', 16, 12, len(entries), 0, 0)
    data += b''.join(struct.pack('<HHII', *entry) for entry in entries)
    data += b'NB05' + I(len(data) + 8)
    assert directory + 16 + len(entries) * 12 + 8 == len(data)
    assert all(offset % 4 == 0 and 8 <= offset < offset + size <= directory for _, _, offset, size in entries)
    return bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    envelope = runpy.run_path(str(Path(__file__).with_name('make-fieldlist-input.py')))
    args.output.write_bytes(envelope['image'](codeview()))
