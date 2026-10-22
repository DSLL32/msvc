#!/usr/bin/env python3
"""Construct small inputs for RC's observed FONT and 2.03 image readers."""
from pathlib import Path
import struct
source = Path(__file__).resolve().parent.parent / 'cases/alternate/source'
# RC reads a native 148-byte directory header; offsets 136 and 140 select
# device and face names. This fixture needs no glyphs: RC stores the file and
# extracts its directory metadata, without validating font rendering data.
font = bytearray(148)
struct.pack_into('<H', font, 0, 0x300)
struct.pack_into('<I', font, 4, 160)
struct.pack_into('<II', font, 136, 0, 148)
font += b'FixtureFont\0'
(source / 'tiny.fnt').write_bytes(font)
# Two legacy forms choose opposite sides of RC's 14 + columns*rows*2 test.
header = bytearray(14)
struct.pack_into('<H', header, 0, 1)
struct.pack_into('<HH', header, 8, 1, 1)
(source / 'legacy.img').write_bytes(header + b'\x12\x34\xaa\xbb\xcc\xdd')
(source / 'inline.img').write_bytes(header)
