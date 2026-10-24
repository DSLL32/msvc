#!/usr/bin/env python3
"""Zero-section COFF weak extern aliases: symbol metadata, never implementation."""
import argparse
from pathlib import Path
import struct


def make_alias(source,target):
    names=source.encode('ascii')+b'\0'+target.encode('ascii')+b'\0'
    header=struct.pack('<HHIIIHH',0x14c,0,0,20,3,0,0)
    weak=struct.pack('<II',0,4)+struct.pack('<IhHBB',0,0,0x20,105,1)
    auxiliary=struct.pack('<II',2,3)+b'\0'*10
    actual=struct.pack('<II',0,5+len(source))+struct.pack('<IhHBB',0,0,0x20,2,0)
    return header+weak+auxiliary+actual+struct.pack('<I',4+len(names))+names


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    parser.add_argument('--delete',action='store_true',help='alias the actual decorated operator-delete import instead')
    args=parser.parse_args()
    args.output.write_bytes(make_alias('_crt_operator_delete','??3@YAXPAX@Z')if args.delete else make_alias('_crt_operator_new','??2@YAPAXI@Z'))
