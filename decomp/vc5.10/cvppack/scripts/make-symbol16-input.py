"""Author NB05 modules comparing C6 register-relative and C7 16-bit symbols.

All records are inputs to the ordinary VC5 CLI, not injected private state.
The two disjoint module contributions remain inside the actual PE text extent.
"""
import argparse
from pathlib import Path
import runpy
import struct

H = lambda n: struct.pack('<H', n)
I = lambda n: struct.pack('<I', n)


def counted(name):
    raw = name.encode('ascii')
    assert len(raw) < 128
    return bytes([len(raw)]) + raw


def old_type(leaf, payload):
    return b'\x01' + H(len(payload) + 1) + bytes([leaf]) + payload


def old_symbol(kind, payload=b''):
    assert len(payload) + 1 < 256
    return bytes([len(payload) + 1, kind]) + payload


def aligned_symbol(kind, payload=b''):
    data = H(0) + H(kind) + payload
    data += b'\0' * (-len(data) % 4)
    return H(len(data) - 2) + data[2:]


def procedure(kind, offset, size, name):
    # S_[LG]PROC16: parent/end/next32, len/debug16, offset/segment/type16,
    # flags byte and counted name. The parent/end links are filled by CVPACK.
    return aligned_symbol(kind, I(0) * 3 + H(size) + H(0) + H(size)
                          + H(offset) + H(1) + H(0x1001) + b'\0' + counted(name))


def records(type_record):
    legacy_types = old_type(0x75, b'\0\x85\x63\0\x83' + I(0xffff))
    legacy = old_symbol(0x11, H(1))
    legacy += old_symbol(1, H(0) + H(0x200) + H(0x80) + H(0) + H(0x80)
                         + H(0) + b'\0' + counted('legacy_reader'))
    regrel = old_symbol(0x16, H(0x10) + b'\x11' + H(0x85) + counted('r'))
    assert len(regrel) == 9
    legacy += regrel + old_symbol(2)
    types = I(2)
    types += type_record(0x1201, I(0))
    types += type_record(0x1008, I(0x74) + b'\0\0' + H(0) + I(0x1000))
    types += type_record(0x1001, I(0x74) + H(1))
    modern = I(2)
    modern += aligned_symbol(0x101, H(0x1c0) + H(1) + H(0x74) + counted('local_word'))
    modern += aligned_symbol(0x102, H(0x1d0) + H(1) + H(0x1002) + counted('global_word'))
    modern += procedure(0x104, 0x100, 0x60, 'adapt')
    modern += aligned_symbol(0x100, H(0xfffc) + H(0x74) + counted('frame'))
    modern += aligned_symbol(6)
    modern += procedure(0x105, 0x180, 0x40, 'service')
    modern += aligned_symbol(0x100, H(4) + H(0x1002) + counted('argument'))
    modern += aligned_symbol(6)
    validate(legacy_types, legacy, types, modern)
    return [(legacy_types, legacy), (types, modern)]


def validate(legacy_types, legacy, types, modern):
    assert legacy_types[:4] == b'\x01' + H(len(legacy_types) - 3) + b'\x75'
    assert legacy_types[4:] == b'\0\x85\x63\0\x83' + I(0xffff)
    cursor, depth = 0, 0
    while cursor < len(legacy):
        length, kind = legacy[cursor:cursor + 2]
        end = cursor + length + 1
        assert end <= len(legacy)
        if kind == 1:
            assert struct.unpack_from('<6H', legacy, cursor + 2) == (0, 0x200, 0x80, 0, 0x80, 0)
            name = cursor + 15
            assert name + 1 + legacy[name] == end
            depth += 1
        elif kind == 2:
            assert length == 1
            depth -= 1
        elif kind == 0x16:
            assert length == 8 and legacy[cursor + 7:end] == counted('r')
            assert struct.unpack_from('<HBH', legacy, cursor + 2) == (0x10, 0x11, 0x85)
        else:
            assert kind == 0x11 and legacy[cursor + 2:end] == H(1)
        assert depth >= 0
        cursor = end
    assert cursor == len(legacy) and depth == 0
    assert types[:4] == modern[:4] == I(2)
    cursor, leaves = 4, []
    while cursor < len(types):
        length, leaf = struct.unpack_from('<HH', types, cursor)
        assert cursor + length + 2 <= len(types) and (length + 2) % 4 == 0
        leaves.append(leaf)
        cursor += length + 2
    assert cursor == len(types) and leaves == [0x1201, 0x1008, 0x1001]
    cursor, depth, visited = 4, 0, set()
    while cursor < len(modern):
        length, kind = struct.unpack_from('<HH', modern, cursor)
        end = cursor + length + 2
        assert end <= len(modern) and (length + 2) % 4 == 0
        visited.add(kind)
        if kind in (0x104, 0x105):
            assert not depth
            parent, finish, following = struct.unpack_from('<III', modern, cursor + 4)
            size, first, last, offset, segment, index = struct.unpack_from('<6H', modern, cursor + 16)
            assert parent == finish == following == first == 0 and last == size
            assert 0x100 <= offset < offset + size <= 0x200 and segment == 1 and index == 0x1001
            name = cursor + 29
            depth += 1
        elif kind in (0x101, 0x102):
            offset, segment, index = struct.unpack_from('<3H', modern, cursor + 4)
            assert 0x100 <= offset < 0x200 and segment == 1 and index in (0x74, 0x1002)
            name = cursor + 10
        elif kind == 0x100:
            assert depth == 1
            offset, index = struct.unpack_from('<HH', modern, cursor + 4)
            assert offset in (0xfffc, 4) and index in (0x74, 0x1002)
            name = cursor + 8
        else:
            assert kind == 6 and length == 2 and depth == 1
            depth -= 1
            cursor = end
            continue
        assert name + 1 + modern[name] <= end
        assert not any(modern[name + 1 + modern[name]:end])
        cursor = end
    assert cursor == len(modern) and depth == 0
    assert visited == {0x100, 0x101, 0x102, 0x104, 0x105, 6}


def codeview(type_record):
    data, rows = bytearray(b'NB05' + I(0)), []
    for number, (types, symbols) in enumerate(records(type_record), 1):
        info = H(0) + H(0) + H(1) + b'CV'
        info += H(1) + H(0) + I((number - 1) * 0x100) + I(0x100)
        info += counted('compat%d.obj' % number)
        for kind, raw in [(0x120, info), (0x121, types), (0x124, symbols)]:
            data += b'\0' * (-len(data) % 4)
            rows.append((kind, number, len(data), len(raw)))
            data += raw
    directory = len(data)
    struct.pack_into('<I', data, 4, directory)
    data += struct.pack('<HHIII', 16, 12, len(rows), 0, 0)
    data += b''.join(struct.pack('<HHII', *row) for row in rows)
    data += b'NB05' + I(len(data) + 8)
    assert len(data) == directory + 16 + 12 * len(rows) + 8
    assert all(8 <= offset < offset + size <= directory for _, _, offset, size in rows)
    return bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    helpers = runpy.run_path(str(Path(__file__).with_name('make-fieldlist-input.py')))
    args.output.write_bytes(helpers['image'](codeview(helpers['record'])))
