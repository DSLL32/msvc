"""Bounded OMF wire serializer controls; explicit original-only analysis mode."""
from pathlib import Path
import argparse,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--original-only',action='store_true');args=ap.parse_args();start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_record@4';row=next(r for r in cfg['driver']if r['name']==name)
if not args.original_only:assert name in cfg['accepted']and row['start']==0x6c440 and row['end']==0x6c4d7,'Writer remains unaccepted; use explicit original-only analysis'
assert original.read(0x6c4d6,1)==b'\xc3'
def run(image,native,used,count,short,theadr=False):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400000;file=0x402000;generated=0x403000;filename=0x403100;u.mem_write(generated,b'compiler generated\0');u.mem_write(filename,b'omftypes.obj\0');u.mem_write(names['_object_file']if native else 0x84fb0,struct.pack('<I',file));u.mem_write(names['_generated_source']if native else 0x85db4,struct.pack('<I',generated));u.mem_write(names['_object_filename']if native else 0x80600,struct.pack('<I',filename));before=bytearray((i*19+7)&255 for i in range(1004));struct.pack_into('<5I',before,0,128,0,0,count,record+20+used)
 if theadr:before[20:34]=b'\x80\0\0\x0aomftypes.c'
 u.mem_write(record,bytes(before));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);fwrite=names['_fwrite']if native else 0x72c30;internal=names['@compiler_internal_error@8']if native else 0x6fb70;error=names['@report_context_error@12']if native else 0x70840;events=[];terminal=None
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal='return';m.emu_stop();return
  if a not in [fwrite,internal,error]:return
  esp=m.reg_read(UC_X86_REG_ESP)
  if a==internal:
   p=m.reg_read(UC_X86_REG_ECX);raw=b''
   for i in range(64):
    c=bytes(m.mem_read(p+i,1));raw+=c
    if c==b'\0':break
   assert raw==b'@(#)omf.c:1.10\0'and m.reg_read(UC_X86_REG_EDX)==0x520;terminal='capacity-error';events.append(dict(kind='internal',code=0x520));m.emu_stop();return
  if a==error:
   assert m.reg_read(UC_X86_REG_ECX)==0x55 and m.reg_read(UC_X86_REG_EDX)==generated and struct.unpack('<I',m.mem_read(esp+4,4))[0]==filename;terminal='write-error';events.append(dict(kind='report',code=0x55));m.emu_stop();return
  pointer,size,n,stream=struct.unpack('<4I',m.mem_read(esp+4,16));assert pointer==record+20 and size==1 and n==used+1 and stream==file
  raw=bytes(m.mem_read(pointer,n));events.append(dict(kind='fwrite',size=size,count=n,file_identity='object_file',wire_hex=raw.hex(),record_count=struct.unpack('<I',m.mem_read(record+12,4))[0],cursor=struct.unpack('<I',m.mem_read(record+16,4))[0]-record))
  if theadr:assert raw==Path('../fixtures/c2-omf-typed-debug/expected/omftypes.obj').read_bytes()[:15]
  m.reg_write(UC_X86_REG_EAX,n-1 if short else n);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x23456789);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else 0x6c440,0,count=10000);assert terminal is not None
 expected=bytearray(before)
 if used>978:assert terminal=='capacity-error'and bytes(u.mem_read(record,1004))==before
 else:
  length=(used-2)&0xffffffff;expected[21]=length&255;expected[22]=length>>8&255;expected[20+used]=-sum(expected[20:20+used])&255;struct.pack_into('<I',expected,16,record+23);struct.pack_into('<I',expected,12,((count+2)&0xffffffff)if short else 0)
  assert terminal==('write-error'if short else'return')and bytes(u.mem_read(record,1004))==expected
  wire=bytes(expected[20:21+used]);assert events[0]==dict(kind='fwrite',size=1,count=used+1,file_identity='object_file',wire_hex=wire.hex(),record_count=(count+2)&0xffffffff,cursor=23)
  if not short:assert u.reg_read(UC_X86_REG_ESP)==0x300804
 return dict(terminal=terminal,events=events,record_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
for used,count,short in itertools.product([0,1,2,3,14,127,255,256,500,978,979],[0,11,0xffffffff],[False,True]):
 a=run(original,False,used,count,short)
 if not args.original_only:assert a==run(canonical,True,used,count,short)
 rows.append(dict(wire_bytes_before_checksum=used,initial_count=count,short_write=short,result=a))
a=run(original,False,14,11,False,True)
if not args.original_only:assert a==run(canonical,True,14,11,False,True)
rows.append(dict(actual_theadr_shape=True,result=a));path=Path('cases/omf-wire-original-analysis.json'if args.original_only else'cases/omf-wire-validation.json');path.write_text(json.dumps(dict(status='original-only analysis, no source/ABI acceptance'if args.original_only else'original/canonical controlled equivalence',states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole151-byte original serializer model: little-endian16 length,8-bit checksum, field counts/cursor/reset, all other pool bytes retained, exact fwrite size1/count/data/object_file pointer; bounded short-write stops at actual typed diagnostic55 and negative capacity at520, neither error is mocked as returning. Degenerate used0..2 are control observations, not actual valid frontend records. Actual THEADR15 bytes independently match frozen original object; native public FILE acceptance still separately required.'),indent=2)+'\n');print(len(rows),'OMF wire controls pass;',path)
