#!/usr/bin/env python3
"""Authored valid COFF symbol input for LINK's older name grammar and truncation.

The shipped CL rejects template source syntax. These are explicit parser inputs,
not claimed compiler output. Grammar follows original demangler token dispatch;
original LINK validation determines the frozen spelling/status of every name.
"""
from pathlib import Path
import struct
import sys

NAMES=['?quoted@@3PM7segment@HM7segment@','?get@?$Box@H@@@QAEHXZ',
       '?value@@3V?Box@H@@@A','?get@?$Box@H@@QAEHXZ','?value@@3V?Box@H@@A','?cut@@3P',
       '?call@@YAHX','?typed@@3V','?argument@@YAHH','?bad@@3V?@']


def make_object(names=NAMES):
    strings=bytearray(4);symbols=bytearray()
    for name in names:
        offset=len(strings)
        strings+=name.encode('ascii')+b'\0\0\0\0'
        symbols+=struct.pack('<IIIhHBB',0,offset,0,1,0,2,0)
    struct.pack_into('<I',strings,0,len(strings))
    header=struct.pack('<HHIIIHH',0x14c,1,0,64,len(names),0,0)
    section=struct.pack('<8sIIIIIIHHI',b'.data',0,0,4,60,0,0,0,0,0xc0000040)
    return header+section+bytes(4)+symbols+strings


if __name__=='__main__':
    output=Path(sys.argv[1]);output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(make_object(sys.argv[2:] or NAMES))
