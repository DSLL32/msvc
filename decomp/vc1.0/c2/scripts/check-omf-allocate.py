"""Actual OMF allocator ABI and complete first-free/eviction controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument("--abi-only",action="store_true");args=ap.parse_args()
start=time.perf_counter();original,canonical,config,_,_=indexes();names=publics();name='@omf_allocate_record@8';row=next(r for r in config['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='type',location='ECX',type='unsigned32'),dict(name='context',location='EDX',type='null32')]
 assert [a['value']for a in e['formals']]==e['original_values']==e['native']['values']==[128,0]
 a=e['pool_input']['original'];b=e['pool_input']['native'];assert a['stride']==1004 and a['count']==4 and a['current']==a['base']and b['current']==b['base']and a['headers']==b['headers']==[bytes(20).hex()]*4
validate(e);reject=[]
for label,mutate in [('missing formal',lambda e:e['formals'].pop()),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('nonnull context',lambda e:e['native']['values'].__setitem__(1,1)),('unknown hidden input',lambda e:e.update(hidden_inputs=['?'])),('wrong current',lambda e:e['pool_input']['native'].update(current=0)),('changed input header',lambda e:e['pool_input']['native']['headers'].__setitem__(0,'01'+bytes(19).hex()))]:
 bad=copy.deepcopy(e);mutate(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
if args.abi_only:
 print("OMF allocation actual ABI/6 rejects pass");raise SystemExit
def run(image,native,type,used,first):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_records']if native else 0x7a8dc;current=names['_omf_current_record']if native else 0x7b88c;init=names['@omf_initialize_record@12']if native else 0x6c060
 before=bytearray((i*29+7)&255 for i in range(4016))
 for i in range(4):struct.pack_into('<I',before,i*1004,0x91 if used>>i&1 else 0)
 u.mem_write(pool,bytes(before));u.mem_write(current,struct.pack('<I',pool+first*1004 if first>=0 else 0));u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,type);u.reg_write(UC_X86_REG_EDX,0)
 expected=next((i for i in range(4)if not used>>i&1),first+1 if 0<=first<3 else 0);events=[]
 def hook(m,a,size,_):
  if a==0x400000:m.emu_stop();return
  if a!=init:return
  esp=m.reg_read(UC_X86_REG_ESP);assert m.reg_read(UC_X86_REG_ECX)==pool+expected*1004 and m.reg_read(UC_X86_REG_EDX)==type and bytes(m.mem_read(esp+4,4))==bytes(4);events.append(expected);m.reg_write(UC_X86_REG_EAX,pool+((expected+2)%4)*1004);m.reg_write(UC_X86_REG_ECX,0x12345);m.reg_write(UC_X86_REG_EDX,0x23456);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+8)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==0x400000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==pool+((expected+2)%4)*1004 and bytes(u.mem_read(pool,4016))==before and events==[expected]
 return expected
rows=[]
for type,used,first in itertools.product([0,128,0xffffffff],range(16),[-1,0,1,2,3,4]):
 a=run(original,False,type,used,first);b=run(canonical,True,type,used,first);assert a==b;rows.append(dict(type=type,used_mask=used,current=first,selected=a))
Path('cases/omf-allocate-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,cases=rows,seconds=time.perf_counter()-start,scope='Whole83-byte original/canonical allocator: all16 occupied masks, current slots0..3 plus numeric below/atend wrap cases, type0/80/FFFFFFFF; first free preferred, otherwise current successor. Mock initializer RET4 returns a deliberately different slot and clobbers caller-save registers. All4016 pool bytes and current pointer unchanged by allocator. Actual first80/NULL observes zero headers/current slot0; no initializer body or cold actual execution claim.'),indent=2)+'\n');print(len(rows),'whole OMF allocation states and6 actual ABI rejects pass')
