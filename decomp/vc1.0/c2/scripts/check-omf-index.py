"""Actual OMF index writer ABI and signed/truncated one/two-byte writes."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_write_index@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());ref=Path(e['original_reference']['directory']);binding=ref/'original-binding.json';assert hashlib.sha256(binding.read_bytes()).hexdigest()==e['original_reference']['binding_sha256'];bound=json.loads(binding.read_text());assert all(hashlib.sha256((ref/n).read_bytes()).hexdigest()==h for n,h in bound['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert e['formals']==[dict(name='record',location='ECX',type='omf_record_current32',value=0),dict(name='index',location='EDX',type='signed32',value=3)]
 assert row['abi']['parameters']==[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='index',location='EDX',type='signed32')]
 assert e['native']['values']==e['original_values']==[0,3]and len(e['record_pointer_formals'])==1
 f=e['record_pointer_formals'][0];assert f['name']=='record'and f['slot']==0 and f['defined_pointee']=='actual28-byte used record'
 for side in ['original','native']:
  r=e['record_input'][side];assert r['pointer']==r['current']==r['base']==f[side+'_pointer']and r['bytes']==28
  b=bytes.fromhex(r['hex']);assert len(b)==28 and struct.unpack('<5I',b[:20])==(153,0,0,5,r['pointer']+28)and b[20:]==b'\x99\x55\0\xa9'+bytes(4)
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong index',lambda e:e['native'].update(values=[0,4])),('unproved hidden register',lambda e:e.update(hidden_inputs=['EAX'])),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong record owner',lambda e:e['record_input']['native'].update(current=0)),('missing record payload',lambda e:e['record_input']['original'].update(hex=bytes(20).hex())),('wrong retained prior length',lambda e:e['record_input']['native'].update(hex=e['record_input']['native']['hex'][:42]+'00'+e['record_input']['native']['hex'][44:]))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c2b0 and row['end']==0x6c2ee and original.read(0x6c2ed,1)==b'\xc3'

def run(image,native,index,count,cursor):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400080;data=bytearray((i*13+5)&255 for i in range(1164));struct.pack_into('<5I',data,128,153,0,0,count,record+cursor);u.mem_write(0x400000,bytes(data));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,index&0xffffffff)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def stop(m,a,size,_):
  if a==0x500000:m.emu_stop()
 u.hook_add(UC_HOOK_CODE,stop);u.emu_start(names[name]if native else row['start'],0,count=200)
 payload=bytes([index&255])if index<128 else bytes([((index>>8)|128)&255,index&255]);expected=bytearray(data);expected[128+cursor:128+cursor+len(payload)]=payload;struct.pack_into('<I',expected,140,(count+len(payload))&0xffffffff);struct.pack_into('<I',expected,144,record+cursor+len(payload))
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x400000,len(data)))==expected
 return dict(payload_hex=payload.hex(),count=(count+len(payload))&0xffffffff,cursor=cursor+len(payload),storage_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
for index,count,cursor in itertools.product([-2147483648,-129,-1,0,3,127,128,255,256,32767,32768,65535,2147483647],[0,0xffffffff],[28,1002]):
 a=run(original,False,index,count,cursor);b=run(canonical,True,index,count,cursor);assert a==b;rows.append(dict(index=index,count=count,cursor=cursor,result=a))
Path('cases/omf-index-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole62-byte original/canonical helper; signed<128 including negative values, high byte truncation/or80, count wrap, cursor and1/2-byte exact writes with full1164-byte record/guard unchanged checks and callee-saved/RET0 ABI. Destination offsets28/1002 remain in1004-byte record; no invented bounds guard or arbitrary adjacent-read/write claim. Actual current pool slot0/index3/count5/cursor28/owned used bytes independently observed; retained previous length55/00 is owned prior state, not consumed by this function. Failed missing-pool/zero-length observer assumptions preserved with original reuse/new native proof.'),indent=2)+'\n');print(len(rows),'whole OMF index writer controls and7 actual ABI rejects pass')
