#!/usr/bin/env python3
"""Independent integer/artifact oracle for the direct relocation lifecycle."""
import argparse
import struct
from pathlib import Path

def expected_rows(payload):
    assert len(payload) == 4112
    page = bytearray(payload[:4096])
    words = payload[4096:]
    rows = []
    for phase, delta in enumerate((0, 0x18004, -0x18004, 0)):
        if phase in (1, 2):
            high, low, full, adj, jump = (struct.unpack_from(f, page, p)[0]
                for f, p in (('<H', 4), ('<H', 6), ('<I', 8), ('<H', 12), ('<I', 16)))
            struct.pack_into('<H', page, 4, (((high << 16) + delta) >> 16) & 0xffff)
            struct.pack_into('<H', page, 6, (low + delta) & 0xffff)
            struct.pack_into('<I', page, 8, (full + delta) & 0xffffffff)
            struct.pack_into('<H', page, 12, (((adj << 16) - 256 + delta + 0x8000) >> 16) & 0xffff)
            displaced = (((jump & 0x3ffffff) << 2) + delta) & 0xffffffff
            struct.pack_into('<I', page, 16, (jump & 0xfc000000) | ((displaced >> 2) & 0x3ffffff))
        result = (0, 14, 14, 0xffffffff)[phase]
        rows.append(struct.pack('<5I', phase, result, 0, 0, 0) + page + words + b'\xa5' * 64)
    return rows

def check(report, payload):
    expected = struct.pack('<5I', 0x31424652, 1, 4, 4096, 16) + payload + b''.join(expected_rows(payload))
    assert len(expected) == 20916
    assert len(report) == len(expected), (len(report), len(expected))
    if report != expected:
        at = next(i for i, (a, b) in enumerate(zip(report, expected)) if a != b)
        raise AssertionError(('report byte mismatch', at, report[at:at+16].hex(), expected[at:at+16].hex()))

if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('report', type=Path)
    ap.add_argument('--payload', type=Path, default=Path('cases/direct-rebase-fallback/inputs/payload.bin'))
    args = ap.parse_args()
    check(args.report.read_bytes(), args.payload.read_bytes())
    print('Four cumulative relocation phases, complete page/words/guards/errors: pass')
