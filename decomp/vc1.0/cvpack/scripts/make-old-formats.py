"""Build a coherent two-module old-CodeView project in two supported dialects.
NB08 is packed C7 (OMF directory, module records, global types/symbol streams).
NB02 is the target's 12-byte-directory C6 dialect. No NB09 bytes are relabelled.
The wire contracts come from the original readers and their decoded call sites;
this is authored format data, not a historical producer-output claim.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

H = lambda n: struct.pack('<H', n)
I = lambda n: struct.pack('<I', n)

def counted(text):
    raw = text.encode('ascii')
    assert len(raw) < 128
    return bytes([len(raw)]) + raw

def aligned(raw):
    return raw + b'\0' * (-len(raw) % 4)

def c7(kind, fields):
    raw = aligned(H(0) + H(kind) + fields)
    return H(len(raw) - 2) + raw[2:]

def c6(kind, fields=b''):
    assert len(fields) + 1 < 256
    return bytes([len(fields) + 1, kind]) + fields

def old_type(leaf, fields):
    return b'\x01' + H(len(fields) + 1) + bytes([leaf]) + fields

def container(signature, rows):
    data = bytearray(signature + I(0))
    entries = []
    for kind, module, raw in rows:
        data += b'\0' * (-len(data) % 4)
        entries.append((kind, module, len(data), len(raw)))
        data += raw
    directory = len(data)
    struct.pack_into('<I', data, 4, directory)
    if signature == b'NB08':
        data += struct.pack('<HHIII', 16, 12, len(entries), 0, 0)
    else:
        data += H(len(entries))
    data += b''.join(struct.pack('<HHII', *e) for e in entries)
    data += signature + I(len(data) + 8)
    validate_container(data, signature)
    return bytes(data), entries

def validate_container(data, signature):
    assert data[:4] == data[-8:-4] == signature
    assert struct.unpack_from('<I', data, len(data) - 4)[0] == len(data)
    directory = struct.unpack_from('<I', data, 4)[0]
    if signature == b'NB08':
        h, e, count, nxt, flags = struct.unpack_from('<HHIII', data, directory)
        assert (h, e, nxt, flags) == (16, 12, 0, 0)
        start = directory + h
    else:
        count = struct.unpack_from('<H', data, directory)[0]
        start = directory + 2
    assert start + count * 12 + 8 == len(data)
    previous = 8
    for i in range(count):
        kind, module, off, size = struct.unpack_from('<HHII', data, start + i * 12)
        assert off >= previous and off + size <= directory
        previous = off + size
        raw = data[off:off + size]
        if signature == b'NB08' and kind == 0x12b:
            flags, n = struct.unpack_from('<II', raw)
            assert flags == 1 and n == 3200
            offsets = struct.unpack_from('<' + 'I' * n, raw, 8)
            cursor = 8 + n * 4
            for index, offset in enumerate(offsets):
                assert offset == cursor
                length, leaf = struct.unpack_from('<HH', raw, cursor)
                assert leaf == 3 and length >= 8
                element, subscript, byte_size = struct.unpack_from('<HHH', raw, cursor + 4)
                assert element == subscript == 0x74 and byte_size == 4 * (index + 1)
                name = cursor + 10
                assert raw[name + 1:name + 1 + raw[name]] == ('span%04d' % index).encode()
                cursor += (length + 5) & ~3
            assert cursor == len(raw)
        elif signature == b'NB08' and kind in (0x129, 0x12a):
            names, addresses, symbols, name_bytes, address_bytes = struct.unpack_from('<HHIII', raw)
            assert not (names or addresses or name_bytes or address_bytes)
            assert 16 + symbols == len(raw)
            validate_c7(raw[16:])
        elif signature == b'NB08' and kind == 0x125:
            assert raw[:4] == I(1)
            validate_c7(raw[4:])
        elif signature == b'NB02' and kind == 0x103:
            cursor = 0
            while cursor < len(raw):
                length = struct.unpack_from('<H', raw, cursor + 1)[0]
                assert raw[cursor] == 1 and raw[cursor + 3] in (0x75, 0x91)
                assert cursor + length + 3 <= len(raw)
                cursor += length + 3
            assert cursor == len(raw)
        elif signature == b'NB02' and kind == 0x104:
            cursor = 0
            while cursor < len(raw):
                length, tag = raw[cursor:cursor + 2]
                assert tag != 0x96 and cursor + length + 1 <= len(raw)
                cursor += length + 1
            assert cursor == len(raw)

def validate_c7(raw):
    cursor = 0
    while cursor < len(raw):
        length, kind = struct.unpack_from('<HH', raw, cursor)
        assert length >= 2 and cursor + length + 2 <= len(raw)
        assert (length + 2) % 4 == 0 and kind in (0x201, 0x202, 0x203)
        segment, type_index = struct.unpack_from('<HH', raw, cursor + 8)
        assert 1 <= segment <= 4 and (type_index == 0x74 or 0x1000 <= type_index < 0x1c80)
        name = cursor + 12
        assert name + 1 + raw[name] <= cursor + length + 2
        cursor += length + 2
    assert cursor == len(raw)

def source_module(number, segments):
    # OMFSourceModule: module ranges, file descriptors, then per-segment line arrays.
    names = ['storage%d.c' % number, 'shared.h']
    data = bytearray(H(2) + H(2) + b'\0' * 8)
    data += b''.join(I(0) + I(0x800) for _ in segments)
    data += b''.join(H(s) for s in segments)
    assert len(data) == 32
    for i, (name, segment) in enumerate(zip(names, segments)):
        struct.pack_into('<I', data, 4 + i * 4, len(data))
        file_at = len(data)
        data += H(1) + H(0) + I(0) + I(0) + I(0x800) + counted(name)
        data += b'\0' * (-len(data) % 4)
        struct.pack_into('<I', data, file_at + 4, len(data))
        data += H(segment) + H(3) + I(0x10) + I(0x40) + I(0x70)
        data += H(10 + i * 30) + H(20 + i * 30) + H(30 + i * 30) + H(0)
    return bytes(data)

def packed_project():
    rows = []
    for number in (1, 2):
        segments = [number * 2 - 1, number * 2]
        module = H(0) + H(1) + H(2) + b'CV'
        module += b''.join(H(s) + H(0) + I(0) + I(0x800) for s in segments)
        module = aligned(module + counted('storage%d.obj' % number))
        symbols = I(1) + b''.join(c7(0x201, I(0x20 + i * 0x20) + H(segments[i % 2]) + H(0x1000 + i * 100) + counted('buffer%d_%d' % (number, i))) for i in range(10))
        rows += [(0x120, number, module), (0x125, number, symbols),
                 (0x127, number, source_module(number, segments))]
    rows.append((0x128, 0xffff, b'\0storage.lib\0'))
    for subsection, kind, prefix in [(0x129, 0x203, 'public'), (0x12a, 0x202, 'global')]:
        symbols = b''.join(c7(kind, I(0x100 + i * 0x20) + H(i + 1) + H(0x1000 + i) + counted(prefix + str(i))) for i in range(4))
        rows.append((subsection, 0xffff, struct.pack('<HHIII', 0, 0, len(symbols), 0, 0) + symbols))
    records = []
    for i in range(3200):
        payload = H(3) + H(0x74) + H(0x74) + H(4 * (i + 1)) + counted('span%04d' % i)
        records.append(aligned(H(len(payload)) + payload))
    header_size = 8 + 4 * len(records)
    cursor = header_size
    offsets = []
    for record in records:
        offsets.append(cursor)
        cursor += len(record)
    types = I(1) + I(len(records)) + b''.join(I(x) for x in offsets) + b''.join(records)
    rows.append((0x12b, 0xffff, types))
    return container(b'NB08', rows)

def unpacked_project():
    rows = []
    references = []
    for number in (1, 2):
        segment = number * 2 - 1
        module = H(segment) + H(0) + H(0x800) + H(0) + H(1) + b'\x02\0' + counted('storage%d.obj' % number) + H(segment + 1) + H(0) + H(0x800)
        symbols = [c6(0x11, H(segment)), c6(0x81, I(0) + H(0x200) + H(0x100) + H(0) + H(0x100) + H(0) + b'\0' + counted('storage_worker'))]
        owners = {}
        for name, raw in [('frame', c6(0x84, I(0xfffedcbc) + H(0x85) + counted('frame_base'))), ('data', c6(0x85, I(0x120) + H(segment) + H(0x85) + counted('data_base'))), ('register', c6(0x0d, H(0x85) + b'\x11' + counted('register_base')))]:
            owners[name] = sum(map(len, symbols))
            symbols.append(raw)
        types = [old_type(0x75, b'\0\x83' + H(0x85) + b'\x63\0\x83' + H(0xffff))]
        for base in (0x93, 0x94, 0x97, 0x98):
            for name, offset in owners.items():
                index = 0x200 + len(types)
                types.append(old_type(0x91, b'\x83' + H(0x85) + bytes([base]) + H(offset)))
                symbols.append(c6(0x0d, H(index) + b'\x11' + counted('base%02x_%s' % (base, name))))
                references.append((number, index, offset))
        symbols.append(c6(2))
        local = b''.join(symbols)
        for module_no, index, offset in references:
            if module_no == number:
                assert local[offset + 1] in (0x84, 0x85, 0x0d)
        lines = b''
        for name, seg in [('storage%d.c' % number, segment), ('shared.h', segment + 1)]:
            lines += counted(name) + H(seg) + H(3) + H(10) + H(0x10) + H(20) + H(0x40) + H(30) + H(0x70)
        rows += [(0x101, number, module), (0x103, number, b''.join(types)),
                 (0x104, number, local), (0x109, number, lines)]
    rows.append((0x106, 0xffff, b'\0storage.lib\0'))
    data, entries = container(b'NB02', rows)
    return data, entries

if __name__ == '__main__':
    destination = Path(sys.argv[1]).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    recipe = {'authored_input': True, 'producer_output': False,
              'contract': 'Recovered original NB08 OMF packed reader and target NB02/C6 converter contracts; all integers little-endian.',
              'excluded': 'LF_BARRAY0x8c and nonadvancing0x96 symbol; no internal state or signature relabelling', 'files': []}
    for filename, builder in [('storage-packed.dbg', packed_project), ('storage-unpacked.dbg', unpacked_project)]:
        data, entries = builder()
        (destination / filename).write_bytes(data)
        recipe['files'].append({'path': filename, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'directory': entries})
        print(filename, len(data), hashlib.sha256(data).hexdigest())
    (destination / 'format-recipe.json').write_text(json.dumps(recipe, indent=2) + '\n')
