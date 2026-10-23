"""Actual empty-name formal and whole index-output wrapper controls."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_name_index@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='name',location='ECX',type='char pointer')]
 assert e['formals'][0]['original']and e['formals'][0]['native']==e['native']['pointer']and e['original_string_hex']==e['native']['hex']=='00'
validate(e);rejected=[]
for label,change in [('missing name',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('hidden output pointer',lambda e:e.update(hidden_inputs=['out?'])),('null name',lambda e:e['formals'][0].update(original=0)),('missing input bytes',lambda e:e['native'].update(hex='')),('different name bytes',lambda e:e['native'].update(hex='4100'))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a940 and row['end']==0x6a971 and original.read(0x6a970,1)==b'\xc3'
def run(image,native,text,found,index,change):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 u.mem_write(0x300000,b'\xa5'*0x10000);u.mem_write(0x400000,text+b'\0');u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,0x400000);children={names[n]if native else a:k for k,n,a in [('intern','@omf_intern_name@8',0x6c540),('begin','@omf_begin_record@8',0x6b480),('string','@omf_append_string@8',0x6c4e0)]};events=[];out=None
 def hook(m,a,size,_):
  nonlocal out
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  kind=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0xdeadbeef
  if kind=='intern':
   assert ecx==0x400000 and 0x300000<=edx<0x300800 and edx%4==0;out=edx;m.mem_write(out,struct.pack('<I',index));result=found;events.append(['intern',text.hex()])
  elif kind=='begin':assert ecx==0x96 and edx==0;events.append(['begin',ecx,edx]);result=0x400200
  else:
   assert ecx==0x400200 and edx==0x400000;events.append(['string',text.hex()])
   if change:m.mem_write(out,struct.pack('<I',(index+11)&0xffffffff))
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x23456789);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000);expected=(index+11)&0xffffffff if change and not found else index
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==expected and len(events)==(1 if found else 3)and bytes(u.mem_read(0x400000,len(text)+1))==text+b'\0'
 return dict(events=events,index=expected)
rows=[]
for text,found,index,change in itertools.product([b'',b'_TEXT',b'abcdefghi',b'\xffx'],[0,1,0xffffffff],[0,1,0xffffffff],[False,True]):
 a=run(original,False,text,found,index,change);b=run(canonical,True,text,found,index,change);assert a==b;rows.append(dict(name_hex=text.hex(),intern_return=found,index=index,append_changes_escaped_local=change,result=a))
Path('cases/omf-name-index-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole49-byte original/canonical wrapper: new/reused/nonzero truth results, exact96/NULL begin then same name pointer, returned record pointer forwarded to string writer, always return4-byte local output after all calls (including explicit mocked escaped-local mutation), poisoned uninitialized stack never seeded as input, exact ESP. Child interner/string/record bodies mocked. Actual nonnull empty NUL input observed independently.'),indent=2)+'\n');print(len(rows),'whole OMF name-index wrapper controls and6 ABI rejects pass')
