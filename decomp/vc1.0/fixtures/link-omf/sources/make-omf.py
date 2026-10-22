"""Construct test input only: TIS1.1 OMF records, NASM-checked FIXUPP bytes."""
from pathlib import Path
import struct,json
D=Path(__file__).resolve().parent
u32=lambda n:struct.pack('<I',n)
def name(s):b=s.encode('ascii');return bytes([len(b)])+b
def rec(kind,data):
 h=bytes([kind])+struct.pack('<H',len(data)+1)+data
 return h+bytes([-sum(h)&255])
# Shared data values exercise native COFF section/symbol/relocation emission.
code=bytes.fromhex('e8 00000000 0305 00000000 0305 00000000 c3')
records=[(0x80,name('omf-probe.asm')),
 (0x96,b''.join(map(name,['','_TEXT','CODE','_DATA','DATA','FLAT','_comdat_value']))),
 (0x99,b'\xa9'+u32(len(code))+b'\x02\x03\x01'),
 (0x99,b'\xa9'+u32(4)+b'\x04\x05\x01'),
 (0x9a,b'\x06\xff\x01\xff\x02'),
 (0x91,b'\x01\x01'+name('_omf_value')+u32(0)+b'\0'),
 (0x91,b'\x01\x02'+name('_data_value')+u32(0)+b'\0'),
 (0x8c,name('_omf_helper')+b'\0'+name('_comdat_value')+b'\0'),
 # COMDAT32, pick-any explicit DATA segment2, dword alignment, zero offset/type, LNAMES7.
 (0xc3,b'\x00\x10\x05'+u32(0)+b'\x00\x01\x02\x07'+u32(15)),
 (0x88,b'\x40\xa2\x01'),
 (0xa1,b'\x01'+u32(0)+code),
 # REL32 external1 at1, DIR32 data segment2 at7, DIR32 external2 at13.
 # FIXDAT selects FLAT group frame1; P=1 omits displacement.
 (0x9d,bytes.fromhex('a401160101 e407140102 e40d160102')),
 (0xa1,b'\x02'+u32(0)+u32(7)),
 (0x8b,b'\0')]
b=b''.join(rec(k,p)for k,p in records);(D/'omf-probe.obj').write_bytes(b)
print('OMF input',len(b),'bytes; records',','.join('%02x'%k for k,p in records))
