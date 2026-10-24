"""Actual four-formal FIXUPP thread ABI and byte-tag/full-result controls."""
from pathlib import Path
import copy,hashlib,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_define_fixup_thread@16';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==8
 expected=[dict(name='flags',location='ECX',type='unsigned8',value=0),dict(name='method',location='EDX',type='unsigned8',value=0),dict(name='target',location='entryESP+4',type='unsigned32',value=3),dict(name='thread',location='entryESP+8',type='unsigned32',value=0)];assert e['formals']==expected and e['native']['values']==e['original_values']==[0,0,3,0]
 for i,f in enumerate(row['abi']['parameters']):assert f==dict(name=expected[i]['name'],location=['CL','DL','entryESP+4','entryESP+8'][i],type=expected[i]['type'])
validate(e);reject=[]
for label,change in [('missing stack formal',lambda e:e['formals'].pop()),('wrong method register',lambda e:e['formals'][1].update(location='entryESP+4')),('wrong target',lambda e:e['native'].update(values=[0,0,4,0])),('wrong thread',lambda e:e['native'].update(values=[0,0,3,1])),('wrong RET cleanup',lambda e:e.update(callee_stack_cleanup=0)),('invented hidden input',lambda e:e.update(hidden_inputs=['EAX']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a800 and row['end']==0x6a843 and original.read(0x6a840,3)==b'\xc2\x08\0'

def run(image,native,flags,method,target,thread,count,upper):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400080;buf=bytearray((i*17+3)&255 for i in range(1004));struct.pack_into('<5I',buf,0,0x9d,0,0,count,record+23);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<4I',0x500000,target,thread,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,flags|(upper<<8));u.reg_write(UC_X86_REG_EDX,method|(upper<<8))
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if native else a:k for n,a,k in [('@omf_begin_record@8',0x6b480,'begin'),('@omf_write_index@8',0x6c2b0,'index')]};events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP)
  if k=='begin':assert ecx==0x9d and edx==0;events.append(['begin',ecx,edx]);result=record
  else:
   assert ecx==record and edx==target and bytes(m.mem_read(record+23,1))==bytes([((method<<2)|thread|flags)&255])and struct.unpack('<I',m.mem_read(record+12,4))[0]==(count+1)&0xffffffff and struct.unpack('<I',m.mem_read(record+16,4))[0]==record+24;events.append(['index',edx]);result=0xdeadbeef
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=300)
 expected=bytearray(buf);expected[23]=((method<<2)|thread|flags)&255;struct.pack_into('<I',expected,12,(count+1)&0xffffffff);struct.pack_into('<I',expected,16,record+24)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x30080c and u.reg_read(UC_X86_REG_EAX)==thread and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(record,1004))==expected and len(events)==2 and bytes(u.mem_read(0x30080c,4))==struct.pack('<I',0xfeedbeef)
 return dict(events=events,tag=expected[23],returned_thread=thread)
shapes=[(0,0,3,0),(0,0,2,1),(0,0,1,2),(0,0,4,3),(0x40,1,1,0),(0xff,0xff,0xffffffff,0xffffffff),(1,0x40,128,0x12345678),(0x80,0x80,0x80000000,0xabcdef02)]
rows=[]
for flags,method,target,thread in shapes:
 for count in [0,0xffffffff]:
  for upper in [0,0xabcdef]:
   a=run(original,False,flags,method,target,thread,count,upper);b=run(canonical,True,flags,method,target,thread,count,upper);assert a==b;rows.append(dict(flags=flags,method=method,target=target,thread=thread,count=count,incoming_upper24=upper,result=a))
Path('cases/omf-thread-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole67-byte original/canonical thread writer; byteCL/DL ignores incoming upper24, methodshift/tagOR/thread truncation, count wrap/cursor exact byte, index request retains full targetbits, full32-bit thread returned despite clobbered child EAX, callee-saved and RET8 stack/sentinel verified.32 controls mock begin/index bodies; actual first flags0/method0/target3/thread0 observed separately, no arbitrary wire semantic validity or completed artifact claim.'),indent=2)+'\n');print(len(rows),'whole OMF thread controls and6 actual ABI rejects pass')
