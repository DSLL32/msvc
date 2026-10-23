"""Actual byte-copy ABI, bounded whole copies and capacity-error frontiers."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_append_bytes@12';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==4
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='bytes',location='EDX',type='char pointer'),dict(name='count',location='entryESP+4',type='unsigned32')]
 a=e['record_input'];b=e['native']['record'];assert e['formals'][0]['original']==a['pointer']==a['current']==a['base']and e['formals'][0]['native']==b['pointer']==b['current']==b['base']
 for r in [a,b]:
  raw=bytes.fromhex(r['hex']);assert len(raw)==24 and struct.unpack('<5I',raw[:20])==(128,0,0,1,r['pointer']+24)and raw[20:]==b'\x80\0\0\x0a'
 assert e['formals'][1]['original']==e['original_source_global']and e['formals'][1]['native']==e['native']['source_global']==e['native']['pointer']and e['original_string_hex']==e['native']['hex']==b'omftypes.c\0'.hex()
 assert e['formals'][2]['original']==e['formals'][2]['native']==e['native']['byte_count']==10
validate(e);rejected=[]
for label,change in [('missing count',lambda e:e['formals'].pop()),('wrong stack',lambda e:e['formals'][2].update(location='entryESP+8')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=0)),('wrong count',lambda e:e['native'].update(byte_count=11)),('unknown hidden input',lambda e:e.update(hidden_inputs=['?'])),('wrong record cursor',lambda e:e['native']['record'].update(hex=bytes(24).hex())),('lost source alias',lambda e:e.update(original_source_global=0))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b650 and row['end']==0x6b69b and original.read(0x6b698,3)==b'\xc2\x04\0'
a=struct.unpack('<I',original.read(0x85074,4))[0];assert original.string(a)=='@(#)omf.c:1.10'
def run(image,native,n,remaining,count,alias=False):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,size in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,size)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400000;offset=998-remaining;cursor=record+offset;source=cursor-2 if alias else 0x402000 if n else 0;data=bytes((i*13+9)&255 for i in range(300));before=bytearray((i*31+7)&255 for i in range(1004));struct.pack_into('<5I',before,0,128,0,0,count,cursor);u.mem_write(record,bytes(before));u.mem_write(0x402000,data);u.mem_write(0x300800,struct.pack('<II',0x500000,n));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,source);error=names['@compiler_internal_error@8']if native else 0x6fb70;errors=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a==error:
   p=m.reg_read(UC_X86_REG_ECX);raw=b''
   for i in range(64):
    c=bytes(m.mem_read(p+i,1));raw+=c
    if c==b'\0':break
   assert raw==b'@(#)omf.c:1.10\0'and m.reg_read(UC_X86_REG_EDX)==0x727;errors.append(0x727);m.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000)
 if remaining<n:
  assert errors==[0x727]and bytes(u.mem_read(record,1004))==before
  return dict(terminal='internal-error',code=errors[0],record_sha256=hashlib.sha256(before).hexdigest())
 assert not errors and u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300808
 expected=bytearray(before)
 for i in range(n):
  struct.pack_into('<I',expected,12,(count+i+1)&0xffffffff);expected[offset+i]=expected[offset-2+i]if alias else data[i];struct.pack_into('<I',expected,16,cursor+i+1)
 assert bytes(u.mem_read(record,1004))==expected and bytes(u.mem_read(0x402000,len(data)))==data
 return dict(terminal='return',record_sha256=hashlib.sha256(expected).hexdigest())
rows=[]
for n,count in itertools.product([0,1,10,127,255],[0,7,0xffffffff]):
 for remaining in [n-1,n,n+1,900,-1]:
  a=run(original,False,n,remaining,count);b=run(canonical,True,n,remaining,count);assert a==b;rows.append(dict(count=n,initial_count=count,remaining=remaining,alias=False,result=a))
for n in [1,3,10,100]:
 a=run(original,False,n,700,7,True);b=run(canonical,True,n,700,7,True);assert a==b;rows.append(dict(count=n,alias=True,result=a))
Path('cases/omf-bytes-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole75-byte original/canonical bounded0..255 copies, NULL/count0, exact capacity boundaries, record-count wrap, forward overlapping source and unchanged tails/source. Capacity failure stops at actual compiler_internal_error source/code, does not pretend error returns or copy oversized ranges. Actual source10 bytes/header/cursor/count stack formal separately proved. No safety or equivalence claim for malformed huge unsigned counts.'),indent=2)+'\n');print(len(rows),'bounded OMF byte-copy/error frontiers and7 ABI rejects pass')
