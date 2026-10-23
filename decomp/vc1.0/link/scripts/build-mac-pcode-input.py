#!/usr/bin/env python3
"""Explicit M68K COFF P-code/native-entry alias fixture input, never executed.

The legacy0x8000 function-type bit and __nep prefix come from native LINK symbol
resolution. Include both special-reference encodings and an ordinary code pointer.
The binary instructions are test input, not reconstruction implementation.
"""
from pathlib import Path
from importlib.machinery import SourceFileLoader
import sys

writer=SourceFileLoader('mac_fixture_writer',str(Path(__file__).with_name('build-mac-static-input.py'))).load_module()


def make_object():
    sections=[('.text',bytes.fromhex('4ead00004e75'),0x60000020,[(2,1,8)]),
              ('.text2',bytes.fromhex('4e750000000080000000'),0x60000020,[(2,1,20),(6,1,20)]),
              ('.data',bytes(4),0xc0000040,[(0,1,3)])]
    symbols=[('_main',0,1,0x20,2),('_pfunc',0,2,0x8020,2),('__nep_pfunc',0,2,0x20,2)]
    return writer.build(sections,symbols)


if __name__=='__main__':
    path=Path(sys.argv[1]);path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(make_object())
