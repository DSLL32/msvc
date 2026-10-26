"""Author a PE32 MIPS-R4000/NB05 image with wide C7 MIPS procedure scopes.

PROCSYMMIPS uses DWORD type/offset fields and WORD segment, from the public
Microsoft CodeView definition. This is authored debug input, not compiler output.
"""
import argparse
from pathlib import Path
import runpy
import struct

H = lambda n: struct.pack('<H', n)
I = lambda n: struct.pack('<I', n)


def counted(name):
    raw = name.encode('ascii')
    assert len(raw) < 256
    return bytes([len(raw)]) + raw


def record(kind, payload=b''):
    data = H(0) + H(kind) + payload
    data += b'\0' * (-len(data) % 4)
    return H(len(data) - 2) + data[2:]


def module(number):
    base = (number - 1) * 256
    info = H(0) + H(0) + H(1) + b'CV' + H(1) + H(0) + I(base) + I(256)
    info += counted('mips%d.obj' % number)
    # LF_ARGLIST(count0), LF_PROCEDURE(void/cdecl, zero arguments).
    types = I(2) + record(0x1201, I(0))
    types += record(0x1008, I(3) + b'\0\0' + H(0) + I(0x1000))
    symbols = bytearray(I(2))
    starts = []
    for i, kind in enumerate((0x1010, 0x1011)):
        start = len(symbols)
        # parent/end/next, length/debug bounds, integer/FP save masks+offsets,
        # DWORD type, DWORD address, WORD section, no return/frame register.
        payload = struct.pack('<10I', 0, 0, 0, 8, 0, 8, 0, 0, 0, 0)
        payload += I(0x1001) + I(base + i * 64) + H(1) + b'\0\0'
        payload += counted(('local' if i == 0 else 'global') + '_mips%d' % number)
        proc = record(kind, payload)
        end = start + len(proc)
        symbols += proc + record(6)
        struct.pack_into('<I', symbols, start + 8, end)
        starts.append(start)
    struct.pack_into('<I', symbols, starts[0] + 12, starts[1])
    cursor, depth, visited = 4, 0, []
    while cursor < len(symbols):
        length, kind = struct.unpack_from('<HH', symbols, cursor)
        end = cursor + length + 2
        assert end <= len(symbols) and end % 4 == 0
        if kind in (0x1010, 0x1011):
            assert depth == 0
            parent, scope_end, following, size, dbg_start, dbg_end, reg, fp, int_off, fp_off = struct.unpack_from('<10I', symbols, cursor + 4)
            assert parent == reg == fp == int_off == fp_off == dbg_start == 0
            assert size == dbg_end == 8 and scope_end == end
            assert following == (starts[1] if kind == 0x1010 else 0)
            index, address, segment, ret_reg, frame_reg = struct.unpack_from('<IIHBB', symbols, cursor + 44)
            assert (index, segment, ret_reg, frame_reg) == (0x1001, 1, 0, 0)
            assert base <= address < address + size <= base + 256
            assert address % 4 == 0 and address - base in (0, 64)
            name_size = symbols[cursor + 56]
            assert cursor + 57 + name_size <= end
            assert symbols[cursor + 57 + name_size:end] == b'\0' * (end - cursor - 57 - name_size)
            visited.append(address)
            depth += 1
        else:
            assert kind == 6 and length == 2 and depth == 1
            depth -= 1
        cursor = end
    assert cursor == len(symbols) and depth == 0 and visited == [base, base + 64]
    assert types[:4] == I(2) and struct.unpack_from('<HHI', types, 4) == (6, 0x1201, 0)
    assert struct.unpack_from('<HHIBBHI', types, 12) == (14, 0x1008, 3, 0, 0, 0, 0x1000)
    return info, types, bytes(symbols)


def image():
    cv, rows = bytearray(b'NB05' + I(0)), []
    for number in (1, 2):
        for kind, data in zip((0x120, 0x121, 0x124), module(number)):
            cv += b'\0' * (-len(cv) % 4)
            rows.append((kind, number, len(cv), len(data)))
            cv += data
    directory = len(cv)
    struct.pack_into('<I', cv, 4, directory)
    cv += struct.pack('<HHIII', 16, 12, len(rows), 0, 0)
    cv += b''.join(struct.pack('<HHII', *r) for r in rows)
    cv += b'NB05' + I(len(cv) + 8)
    assert all(offset % 4 == 0 and 8 <= offset < offset + size <= directory for _, _, offset, size in rows)
    envelope = runpy.run_path(str(Path(__file__).with_name('make-fieldlist-input.py')))
    data = bytearray(envelope['image'](bytes(cv)))
    struct.pack_into('<H', data, 0x84, 0x166)  # IMAGE_FILE_MACHINE_R4000
    data[0x200:0x400] = b'\0' * 512
    for address in (0, 64, 256, 320):
        # MIPS little-endian JR RA and the mandatory NOP delay slot.
        data[0x200 + address:0x208 + address] = I(0x03e00008) + I(0)
    assert struct.unpack_from('<H', data, 0x84)[0] == 0x166
    assert all(struct.unpack_from('<II', data, 0x200 + a) == (0x03e00008, 0) for a in (0, 64, 256, 320))
    assert data[0x41c:0x420] == b'NB05' and data[0x41c:0x41c + len(cv)] == cv
    return bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_bytes(image())
