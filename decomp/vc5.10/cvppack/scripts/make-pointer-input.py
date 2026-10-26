"""Author a C6 pointer-addressing model in a coherent PE32/NB05 input.

Microsoft cvdump/typeinfo.h and type6.cpp define near/far/huge pointer names,
BASESEG/BASEVAL/BASESEGVAL/BASEADR/BASESEGADR and their WORD symbol offsets.
Those offsets refer to real records in this input's local-symbol subsection.
VC5's actually consumed symbol widths are retained, including DWORD type fields
in wide BPREL/DATA. No output attributes or original stack bytes are prescribed.
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


def typ(leaf, payload):
    return b'\1' + H(len(payload) + 1) + bytes([leaf]) + payload


def sym(kind, payload=b''):
    assert len(payload) < 254
    return bytes([len(payload) + 1, kind]) + payload


def schema(module):
    start = (module - 1) * 0x100
    symbols = sym(0x11, H(1))
    symbols += sym(1, H(start) + H(0x200) + H(0x80) + H(0) + H(0x80)
                   + H(0) + b'\0' + counted('address_model'))
    base_rows = [(0x0d, H(0x85) + b'\x11' + counted('regbase')),
                 (4, H(4) + H(0x85) + counted('framebase')),
                 (5, H(start + 0x90) + H(1) + H(0x85) + counted('database')),
                 (0x84, I(8) + I(0x86) + counted('wideframe')),
                 (0x85, I(0x86) + I(start + 0xa0) + H(1) + counted('widedata'))]
    offsets = []
    for kind, payload in base_rows:
        offsets.append(len(symbols))
        symbols += sym(kind, payload)
    records = [('procedure', typ(0x75, b'\0\x85\x63\0\x83' + I(0xffff)))]
    for tag, name in [(0x74, 'near'), (0x73, 'far'), (0x5e, 'huge')]:
        record = typ(0x7a, bytes([tag, 0x83]) + H(0x85) + b'\x82' + counted(name + '_name'))
        assert len(record) >= 12  # original in-place LF_POINTER expansion
        records.append((name, record))
    records += [('segment', typ(0x91, b'\x83' + H(0x85) + b'\x92' + H(1))),
                ('self', typ(0x91, b'\x83' + H(0x85) + b'\x81')),
                ('type', typ(0x91, b'\x83' + H(0x85) + b'\x83' + H(0x85)))]
    for n, offset in enumerate(offsets):
        # Exercise actual base-value, segment-value, address and segment-address
        # relationships. Reserved WORD is zero as defined by the C6 format.
        tag = [0x93, 0x94, 0x97, 0x98, 0x93][n]
        records.append(('based%d' % n, typ(0x91, b'\x83' + H(0x85) + bytes([tag]) + H(offset) + H(0))))
    for n, (name, _) in enumerate(records[1:], 1):
        symbols += sym(0x0d, H(0x200 + n) + b'\x11' + counted(name))
    symbols += sym(2)
    validate(records, symbols, offsets, module)
    return b''.join(raw for _, raw in records), symbols


def validate(records, symbols, offsets, module):
    assert len(records) == 12
    for n, (name, raw) in enumerate(records):
        marker, length, leaf = struct.unpack_from('<BHB', raw)
        assert marker == 1 and length + 3 == len(raw)
        if n == 0:
            assert raw[3:] == b'\x75\0\x85\x63\0\x83' + I(0xffff)
        elif leaf == 0x7a:
            assert len(raw) >= 12 and raw[4] in (0x74, 0x73, 0x5e)
            assert raw[5:8] == b'\x83' + H(0x85) and raw[8] == 0x82
            assert 10 + raw[9] == len(raw)
        else:
            assert leaf == 0x91 and raw[4:7] == b'\x83' + H(0x85)
            tag = raw[7]
            if tag in (0x92, 0x83):
                assert len(raw) == 10 and raw[8:] == H(1 if tag == 0x92 else 0x85)
            elif tag == 0x81:
                assert len(raw) == 8
            else:
                offset, reserved = struct.unpack_from('<HH', raw, 8)
                assert len(raw) == 12 and offset in offsets and reserved == 0
                assert symbols[offset + 1] in (0x0d, 4, 5, 0x84, 0x85)
    cursor, depth, references = 0, 0, set()
    while cursor < len(symbols):
        length, kind = symbols[cursor:cursor + 2]
        end = cursor + length + 1
        assert length and end <= len(symbols)
        if kind == 1:
            assert depth == 0
            assert struct.unpack_from('<6H', symbols, cursor + 2) == ((module - 1) * 0x100, 0x200, 0x80, 0, 0x80, 0)
            depth += 1
            name, position, width = cursor + 15, cursor + 4, 2
        elif kind == 0x0d:
            name, position, width = cursor + 5, cursor + 2, 2
        elif kind in (4, 0x84):
            wide = kind == 0x84
            name, position, width = cursor + (10 if wide else 6), cursor + (6 if wide else 4), 4 if wide else 2
        elif kind in (5, 0x85):
            wide = kind == 0x85
            name, position, width = cursor + (12 if wide else 8), cursor + (2 if wide else 6), 4 if wide else 2
        elif kind == 2:
            depth -= 1
            assert length == 1 and depth == 0
            cursor = end
            continue
        else:
            assert kind == 0x11 and symbols[cursor + 2:end] == H(1)
            cursor = end
            continue
        assert name + 1 + symbols[name] == end
        index = int.from_bytes(symbols[position:position + width], 'little')
        assert index in (0x85, 0x86) or 0x200 <= index < 0x200 + len(records)
        references.add(index)
        cursor = end
    assert cursor == len(symbols) and depth == 0
    assert references >= set(range(0x200, 0x200 + len(records)))


def codeview():
    data, entries = bytearray(b'NB05' + I(0)), []
    for module in (1, 2):
        types, symbols = schema(module)
        info = H(0) + H(0) + H(1) + b'CV' + H(1) + H(0)
        info += I((module - 1) * 0x100) + I(0x100) + counted('address%d.obj' % module)
        for kind, raw in [(0x120, info), (0x121, types), (0x124, symbols)]:
            data += b'\0' * (-len(data) % 4)
            entries.append((kind, module, len(data), len(raw)))
            data += raw
    directory = len(data)
    struct.pack_into('<I', data, 4, directory)
    data += struct.pack('<HHIII', 16, 12, len(entries), 0, 0)
    data += b''.join(struct.pack('<HHII', *row) for row in entries)
    data += b'NB05' + I(len(data) + 8)
    assert directory + 16 + 12 * len(entries) + 8 == len(data)
    assert all(offset % 4 == 0 and 8 <= offset < offset + size <= directory for _, _, offset, size in entries)
    return bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    envelope = runpy.run_path(str(Path(__file__).with_name('make-fieldlist-input.py')))
    args.output.write_bytes(envelope['image'](codeview()))
