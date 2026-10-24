"""Actual write-only destination ownership and whole bounded zero-padding loop."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_pad_record@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='bytes',location='EDX',type='unsigned32')]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b2a0'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 assert e['shape']==dict(slot=0,bytes=2,count=38,cursor_offset=66)and e['original']['return_address']==0x6bf15 and e['native']['caller']=='omf_find_record'
 for side in ['original','native']:
  d=e[side];assert d['record']==d['current']==d['base']and d['bytes']==2 and d['count']==38 and d['cursor']==d['record']+66
  assert [f[side]for f in e['formals']]==[d[k]for k in ['record','bytes']]
validate(e);reject=[]
for label,change in [('missing bytes formal',lambda e:e['formals'].pop()),('wrong count location',lambda e:e['formals'][1].update(location='entryESP+4')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong record owner',lambda e:e['original'].update(current=0)),('wrong cursor',lambda e:e['native'].update(cursor=0)),('wrong prior count',lambda e:e['native'].update(count=0)),('wrong requested count',lambda e:e['original'].update(bytes=3)),('invented destination old input',lambda e:e.update(hidden_inputs=['old_destination']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b2a0 and row['end']==0x6b2bc and original.read(0x6b2bb,1)==b'\xc3'
def run(image,native,offset,count,requested):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400080;buf=bytearray((i*17+3)&255 for i in range(1004));struct.pack_into('<2I',buf,12,count,record+offset);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,requested);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 u.emu_start(names[name]if native else row['start'],0x500000,count=10000)
 struct.pack_into('<2I',buf,12,(count+requested)&0xffffffff,record+offset+requested);buf[offset:offset+requested]=bytes(requested)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(record,1004))==buf
 return dict(count=(count+requested)&0xffffffff,cursor_offset=offset+requested,record_sha256=hashlib.sha256(buf).hexdigest())
inputs=list(itertools.product([20,66,500],[0,38,0xffffffff],[0,1,2,17,256]))+[(998,0xfffffff0,n)for n in [0,1,2,6]];rows=[]
for offset,count,requested in inputs:
 a=run(original,False,offset,count,requested);b=run(canonical,True,offset,count,requested);assert a==b;rows.append(dict(cursor_offset=offset,initial_count=count,requested=requested,result=a))
Path('cases/omf-pad-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole28-byte original/canonical loop, no child calls:0/1/2/17/256 requests, five bounded cursor positions including exact1004 end, unsigned count wrap, zero-only destination writes and unchanged full owned-record guards, RET0/callee-saved/stack checked against independent bytes/count/cursor expectation. Synthetic destination patterns are overwrite guards, not captured old input. Actual request2/count38/cursor66/current slot0 observed; caller originalreturn6bf15/nativeomf_find_record. No unbounded/oversized request success, capacity reservation or invisible destination old bytes assumed.'),indent=2)+'\n');print(len(rows),'whole zero-pad controls and8 actual ABI rejects pass')
