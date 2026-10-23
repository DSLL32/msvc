#!/usr/bin/env python3
"""Explicit COFF duplicated-contribution fixture input, not a runnable Mac test.

DUPCON16(type41) in startup requests a per-CODE-resource copy. Its call field
uses CTOC32(type12), causing native loader fixup bookkeeping for the copies.
"""
from pathlib import Path
from importlib.machinery import SourceFileLoader
import sys
writer=SourceFileLoader('mac_fixture_writer',str(Path(__file__).with_name('build-mac-static-input.py'))).load_module()


def make_object():
    sections=[('.text',bytes.fromhex('4e7100004e754e75'),0x60000020,[(2,3,41)]),
              ('.text2',bytes.fromhex('4e75'),0x60000020,[]),
              ('.dup',bytes.fromhex('4eb9000000004e75'),0x60000020,[(2,2,12)])]
    symbols=[('_main',0,1,0x20,2),('___Init32BitLoadSeg',6,1,0x20,2),
             ('_helper',0,2,0x20,2),('_copyfn',0,3,0x20,2)]
    return writer.build(sections,symbols)


if __name__=='__main__':
    path=Path(sys.argv[1]);path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(make_object())
