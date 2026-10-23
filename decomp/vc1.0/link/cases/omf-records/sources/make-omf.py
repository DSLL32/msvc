"""Generate valid OMF test inputs; no reconstruction implementation bytes.

TIS OMF1.1 COMDEF/LCOMDEF, BAKPAT, NBKPAT, LINNUM and LPUBDEF.
Existing tested Microsoft explicit-allocation COMDAT convention is retained.
"""
from pathlib import Path
import json
import struct

HERE=Path(__file__).resolve().parent
u16=lambda n:struct.pack('<H',n)
u32=lambda n:struct.pack('<I',n)
def name(s):
    data=s.encode('ascii')
    return bytes([len(data)])+data
def record(kind,data):
    header=bytes([kind])+u16(len(data)+1)+data
    return header+bytes([-sum(header)&255])
def common(s,size,encoding=0x00,far=False):
    length=bytes([size]) if not encoding else bytes([encoding])+size.to_bytes({0x81:2,0x84:3,0x88:4}[encoding],'little')
    return name(s)+b'\0'+(b'\x61'+length+b'\x04' if far else b'\x62'+length)
#72 native instruction offsets accommodate70 monotonically increasing lines.
code=b'\xb8'+u32(42)+b'\x90'*70+b'\xc3'
data=bytes.fromhex('11 00 22 00 33 00 00 00')
comdat=bytes.fromhex('21 00 32 00 43 00 00 00')
records=[(0x80,name('records.asm')),
 (0x96,b''.join(map(name,['','_TEXT','CODE','_DATA','DATA','FLAT','_patched_comdat']))),
 (0x99,b'\xa9'+u32(len(code))+b'\x02\x03\x01'),
 (0x99,b'\xa9'+u32(len(data))+b'\x04\x05\x01'),
 (0x9a,b'\x06\xff\x01\xff\x02'),
 (0x91,b'\x01\x01'+name('_omf_entry')+u32(0)+b'\0'),
 (0x91,b'\x01\x02'+name('_patched_data')+u32(0)+b'\0'),
 (0xb0,common('_near_small',8)+common('_near_word',0x123,0x81)+common('_near_24',0x12345,0x84)+common('_near_32',0x23456,0x88)+common('_far_array',257,0x81,True)),
 (0xb8,common('_local_common',16)),
 (0xb7,b'\x01\x01'+name('local_code')+u32(5)+b'\0'),
 (0xb6,b'\x01\x02'+name('local_data')+u16(0)+b'\0'),
 (0xa1,b'\x01'+u32(0)+code),
 (0xa1,b'\x02'+u32(0)+data),
 (0xb3,b'\x02\x00'+u32(0)+u32(1)),
 (0xb3,b'\x02\x01'+u32(2)+u32(2)),
 (0xb3,b'\x02\x02'+u32(4)+u32(3)),
 (0xc3,b'\x00\x10\x05'+u32(0)+b'\x00\x01\x02\x07'+comdat),
 (0xc9,b'\x00\x07'+u32(0)+u32(1)),
 (0xc9,b'\x01\x07'+u32(2)+u32(2)),
 (0xc9,b'\x02\x07'+u32(4)+u32(3)),
 (0x95,b'\x01\x01'+b''.join(u16(i+100)+u32(5+i) for i in range(70))),
 (0x8b,b'\0')]
image=b''.join(record(k,data) for k,data in records)
out=HERE.parent/'inputs/records.obj';out.write_bytes(image)
(HERE.parent/'inputs/layout.json').write_text(json.dumps(dict(records=[dict(type=hex(k),payload_bytes=len(p)) for k,p in records],code_bytes=len(code),data_before=data.hex(),comdat_before=comdat.hex(),line_records=70,common_sizes=[8,0x123,0x12345,0x23456,1028,16],sources=['https://refspecs.linuxfoundation.org/elf/TIS1.1.pdf','https://www.bitsavers.org/pdf/ibm/pc/os2/OS2_OMF_and_LX_Object_Formats_Revision_8_199406.pdf'],native_quirks='Native BAKPAT/NBKPAT operation is2*existing+addend; independent original output must establish actual values'),indent=2)+'\n')
print(out,len(image))
