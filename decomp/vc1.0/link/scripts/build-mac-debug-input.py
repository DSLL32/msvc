#!/usr/bin/env python3
"""Derive an explicit COFF test input from the frozen loader/relocation corpus.

Append a public ___InitSwapper marker at an existing RTS, retaining every old
section and relocation byte. This generates fixture input, never implementation.
"""
import argparse
import hashlib
from pathlib import Path
import struct

BINARY=Path(__file__).resolve().parents[1]


def make_object():
    base=(BINARY/'cases/m68k-reloc-loader/inputs/mac-reloc.obj').read_bytes()
    assert hashlib.sha256(base).hexdigest()=='ee86b6debb342172329b06b92b456cb237f1afc2c63c70dffb80788e11f03951'
    symbol,count=struct.unpack_from('<II',base,8)
    offset=symbol+18*count
    strings=bytearray(base[offset:])
    name_offset=len(strings)
    strings+=b'___InitSwapper\0'
    struct.pack_into('<I',strings,0,len(strings))
    result=bytearray(base[:offset])
    result+=struct.pack('<IIIhHBB',0,name_offset,6,1,0x20,2,0)
    result+=strings
    struct.pack_into('<I',result,12,count+1)
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    options=p.parse_args()
    data=make_object()
    options.output.parent.mkdir(parents=True,exist_ok=True)
    options.output.write_bytes(data)
    print(options.output,len(data),hashlib.sha256(data).hexdigest())


if __name__=='__main__':
    main()
