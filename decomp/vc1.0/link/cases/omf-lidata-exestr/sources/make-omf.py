"""Generate ordinary TIS OMF LIDATA16/32 and EXESTR input records.

Fixture data only, not reconstruction implementation. Record formats:
https://refspecs.linuxfoundation.org/elf/TIS1.1.pdf
LIDATA is A2/A3: segment index, offset, recursively repeated data blocks.
COMENT88 classA4 is an executable string. Each record has an OMF checksum.
"""
from pathlib import Path
import struct,json
HERE=Path(__file__).resolve().parent
w=lambda n:struct.pack('<H',n)
d=lambda n:struct.pack('<I',n)
def name(s):
    data=s.encode('ascii');return bytes([len(data)])+data
def record(kind,payload):
    body=bytes([kind])+w(len(payload)+1)+payload
    return body+bytes([-sum(body)&255])
def leaf(count,data,wide=False):return (d(count) if wide else w(count))+w(0)+bytes([len(data)])+data
def group(count,*blocks,wide=False):return (d(count) if wide else w(count))+w(len(blocks))+b''.join(blocks)
block16=group(2,leaf(3,b'AB'),group(1,leaf(2,b'1'),leaf(1,b'XYZ')))+leaf(4,b'\x10\x20')
block32=group(3,leaf(2,b'c\0',True),wide=True)+leaf(2,b'DWORD!',True)
expected=(b'AB'*3+b'11XYZ')*2+b'\x10\x20'*4+b'\0'*10+b'c\0'*6+b'DWORD!'*2
assert len(expected)==64
comments=[b'compiler vc1',b'fixture-data',b'third comment']
code=bytes.fromhex('b800000000c3')
records=[(0x80,name('lidata.asm')),
 (0x96,b''.join(map(name,['','_TEXT','CODE','_DATA','DATA','FLAT']))),
 (0x99,b'\xa9'+d(len(code))+b'\x02\x03\x01'),
 (0x99,b'\xa9'+d(len(expected))+b'\x04\x05\x01'),
 (0x9a,b'\x06\xff\x01\xff\x02'),
 (0x91,b'\x01\x01'+name('_lidata_entry')+d(0)+b'\0'),
 (0x91,b'\x01\x02'+name('_lidata_data')+d(0)+b'\0'),
 (0xa1,b'\x01'+d(0)+code),
 (0xa2,b'\x02'+w(0)+block16),
 (0xa3,b'\x02'+d(40)+block32),
 *[(0x88,b'\x00\xa4'+s) for s in comments],
 (0x8b,b'\0')]
image=b''.join(record(k,p) for k,p in records)
(HERE.parent/'inputs/lidata.obj').write_bytes(image)
(HERE.parent/'inputs/layout.json').write_text(json.dumps(dict(records=[dict(type=hex(k),payload_bytes=len(p)) for k,p in records],expected_data_hex=expected.hex(),comments=[s.decode() for s in comments],source='TIS OMF1.1 LIDATA/COMENT-EXESTR; successful untouched-original artifacts must establish actual native comment serialization separately'),indent=2)+'\n')
print(len(image),'OMF bytes,',len(expected),'expanded data bytes')
