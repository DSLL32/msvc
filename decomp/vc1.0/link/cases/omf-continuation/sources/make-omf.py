"""Valid documented OMF fixtures, never reconstruction implementation.

TIS OMF1.1 pp38(USE16),43-46(FIXUPP),61-65(COMDAT/LINSYM).
Microsoft's LNAMES index variant is explicitly documented for LINSYM/COMDAT.
"""
import hashlib,json,struct
from pathlib import Path
BASE=Path(__file__).resolve().parents[1]
w=lambda v:struct.pack('<H',v)
d=lambda v:struct.pack('<I',v)
def name(value):
    data=value.encode('ascii');return bytes([len(data)])+data
def record(kind,data):
    image=bytes([kind])+w(len(data)+1)+data
    return image+bytes([-sum(image)&255])
# Three actual32-bit absolute references, two share target+4 and one targets+8.
code=bytes.fromhex('a1000000008b0d000000008b1500000000c3')
data=bytes.fromhex('11223344 55667788 99aabbcc ddeeff00')
comdat=bytes.fromhex('0102030405060708 1112131415161718')
records=[(0x80,name('continue.asm')),
 (0x96,b''.join(map(name,['','_TEXT','CODE','_DATA','DATA','FLAT','_continued']))),
 (0x99,b'\xa9'+d(len(code))+b'\x02\x03\x01'),
 (0x99,b'\xa9'+d(len(data))+b'\x04\x05\x01'),
 (0x9a,b'\x06\xff\x01\xff\x02'),
 (0x91,b'\x01\x01'+name('_omf_entry')+d(0)+b'\0'),
 (0x91,b'\x01\x02'+name('_target_data')+d(0)+b'\0'),
 (0xa1,b'\x01'+d(0)+code),
 (0x9d,b''.join(b'\xe4'+bytes([offset])+b'\x50\x02'+d(displacement) for offset,displacement in [(1,4),(7,4),(13,8)])),
 (0xa1,b'\x02'+d(0)+data),
 (0xc3,b'\x00\x10\x05'+d(0)+b'\x00\x01\x02\x07'+comdat[:8]),
 (0xc3,b'\x01\x10\x05'+d(8)+b'\x00\x01\x02\x07'+comdat[8:]),
 (0xc4,b'\0\x07'+w(101)+w(0)+w(102)+w(4)),
 (0xc5,b'\x01\x07'+w(103)+d(8)+w(104)+d(12)),
 (0x8b,b'\0')]
for variant in ['continued','use16']:
    actual=records[:]
    if variant=='use16':
        # Ordinary USE16 data, not a16bit entrypoint or reserved BIG segment.
        actual[3]=(0x99,b'\xa8'+d(len(data))+b'\x04\x05\x01')
    image=b''.join(record(k,p) for k,p in actual);(BASE/'inputs'/(variant+'.obj')).write_bytes(image)
    offset=0;layout=[]
    for k,p in actual:
        raw=record(k,p);assert sum(raw)&255==0
        layout.append(dict(offset=offset,type=hex(k),payload_bytes=len(p),checksum=raw[-1]));offset+=len(raw)
    (BASE/'inputs'/(variant+'-layout.json')).write_text(json.dumps(dict(sha256=hashlib.sha256(image).hexdigest(),bytes=len(image),records=layout,code_hex=code.hex(),data_hex=data.hex(),continued_comdat_hex=comdat.hex(),fixups=[dict(source_offset=a,target_segment=2,target_displacement=b) for a,b in [(1,4),(7,4),(13,8)]],source='https://openwatcom.org/ftp/devel/docs/omf.pdf',source_pages=[38,43,44,45,46,61,62,63,64,65],scope='Ordinary valid explicit allocation COMDAT, continuationflag1;Microsoft LNAMES LINSYM;FIXUP32 segmentrelative32bitlocation/frame5/targetsegment2/nonzerooffset; USE16 companion changes onlyPbit, not recordwidth.'),indent=2)+'\n')
    spec=dict(case='omf-continuation-'+variant,arguments=['-lib','/out:'+variant+'.lib',variant+'.obj'],environment=dict(LIB='{lib}',LINK32_DB='27'),inputs=[dict(path=str(BASE.relative_to(Path.cwd())/'inputs'/(variant+'.obj')),bytes=len(image),sha256=hashlib.sha256(image).hexdigest())],outputs=[dict(output_name=variant+'.lib',artifact_kind='archive-generated')],final_files=[variant+'.lib',variant+'.obj'])
    (BASE/(variant+'-spec.json')).write_text(json.dumps(spec,indent=2)+'\n')
    print(variant,len(image))
