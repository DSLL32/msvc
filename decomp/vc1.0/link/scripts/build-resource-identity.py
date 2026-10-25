#!/usr/bin/env python3
"""Explicit valid Win32 RES fixture records, not reconstruction code.

DWORD data/header sizes, UTF16 type/name, DWORD-aligned16-byte fixed attributes
follow NT-era Win32 RES layouts. Distinct language IDs make the positive records
unique; the companion changes one language only to create an ordinary duplicate
resource identity. Record bytes/checksums are never repaired after capture.
"""
from pathlib import Path
import hashlib,json,struct,shutil,sys

def record(language,payload):
    names=('SAMPLE\0'+'Caf\u00e9\0').encode('utf-16le')
    prefix=bytes(8)+names
    prefix+=bytes(-len(prefix)%4)
    header=bytearray(prefix+struct.pack('<IHHII',0,0x30,language,0,0))
    struct.pack_into('<II',header,0,len(payload),len(header))
    data=bytes(header)+payload;return data+bytes(-len(data)%4)
if __name__=='__main__':
    b=Path(sys.argv[1]);b.mkdir(parents=True,exist_ok=True)
    empty=struct.pack('<IIHHHHIHHII',0,32,0xffff,0,0xffff,0,0,0,0,0,0)
    records=[(0x409,b'English identity payload\0'),(0x411,b'Japanese identity payload\0')]
    for name,language in [('languages',0x411),('duplicate',0x409)]:
        d=b/name;d.mkdir(exist_ok=True);data=empty+record(records[0][0],records[0][1])+record(language,records[1][1]);(d/'identity.res').write_bytes(data)
        (d/'layout.json').write_text(json.dumps(dict(format='Win32 RES, two named records following32-byte NULL record',type='SAMPLE',name='Caf\u00e9',languages=[0x409,language],payloads=[p.decode('ascii') for _,p in records],bytes=len(data),sha256=hashlib.sha256(data).hexdigest()),indent=2)+'\n')
    if len(sys.argv)>2:shutil.copyfile(Path(sys.argv[2]),b/'test.obj')
