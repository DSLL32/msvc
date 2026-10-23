"""Actual OMF record factory inputs and whole dependency-flush/reuse controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--abi-only',action='store_true');args=parser.parse_args();started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='@omf_begin_record@8';row=next(r for r in config['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(r):
 assert r['matched']and r['function']==name and r['profile']=='omf'and r['no_hidden_inputs_proof']
 assert [{k:a[k]for k in ['name','location','type']}for a in r['formals']]==row['abi']['parameters']==[dict(name='type',location='ECX',type='unsigned32'),dict(name='context',location='EDX',type='null32')]
 assert [a['value']for a in r['formals']]==r['original_values']==r['native']['values']==[0x80,0]and r['null_pointer_formals']==['context']
 assert r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(e);rejected=[]
for label,change in [('missing context',lambda r:r['formals'].pop()),('wrong context register',lambda r:r['formals'][1].update(location='entryESP+4')),('nonNULL actual context',lambda r:r['native']['values'].__setitem__(1,1)),('wrong actual type',lambda r:r['native']['values'].__setitem__(0,0xc3)),('missing NULL pointer proof',lambda r:r.update(null_pointer_formals=[])),('unknown hidden argument',lambda r:r.update(hidden_inputs=['out?']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
Path('cases/omf-begin-abi-validation.json').write_text(json.dumps(dict(actual_formals=e['formals'],null_pointer_formals=e['null_pointer_formals'],rejected=rejected,seconds=time.perf_counter()-started,scope='Actual first ECX80 and EDX NULL, no pointee input. This does not claim NULL valid for record typeC3, which reads context+4.'),indent=2)+'\n')
if args.abi_only:print('2 actual OMF factory inputs and6 ABI rejects pass');raise SystemExit(0)
def run(image,side,type,context_value,pattern,found):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0];put=lambda a,v:u.mem_write(a,struct.pack('<I',v&0xffffffff));context=0 if context_value is None else 0x204000
 if context:put(context,0xdeadbeef);put(context+4,context_value)
 current=names['_omf_current_record']if side else 0x7b88c;put(current,0x205000);put(0x300800,0x400000);u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,type);u.reg_write(UC_X86_REG_EDX,context)
 calls={names[n]if side else a:k for k,n,a in [('flush','@omf_flush_record_type@4',0x6aaf0),('find','@omf_find_record@8',0x6bd50),('allocate','@omf_allocate_record@8',0x6c250)]};events=[];flushes=[0]
 def hook(m,a,size,unused):
  if a==0x400000:m.emu_stop();return
  if a not in calls:return
  k=calls[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP)
  if k=='flush':
   result=(pattern>>flushes[0])&1;flushes[0]+=1;events.append(['flush',ecx,result])
   if type==0xc3 and ecx==0xa1 and pattern&4:put(context+4,context_value^1)
  else:
   assert ecx==type and edx==context;events.append([k,ecx,edx]);result=0x206000 if found else 0
   if k=='allocate':assert not found;result=0x207000
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=10000);expected=0x206000 if found else 0x207000;assert u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==word(current)==expected
 return dict(events=events,result=expected,context_value=word(context+4)if context else None)
rows=[]
for type,context,pattern,found in itertools.product([0,0x80,0x8c,0x91,0x96,0xa1,0xb0,0xb4,0xb7,0xbc,0xc3,0xca,0xffffffff],[None,0,1],range(8),[0,1]):
 if type==0xc3 and context is None:continue
 a=run(original,False,type,context,pattern,found);b=run(canonical,True,type,context,pattern,found);assert a==b,(type,context,pattern,found,a,b);rows.append(dict(type=hex(type),context=context,pattern=pattern,found=found,events=a['events']))
Path('cases/omf-begin-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,cases=rows,seconds=time.perf_counter()-started,scope='Whole463-byte original/canonical dependency-flush factory: all11 explicit record types, two default keys, three context shapes, eight mocked flush-result patterns and reuse/allocation. TypeC3 uses real defined context+4 and child-mock mutation proves load afterA1 flush; NULL C3 excluded because original dereferences it, no invented guard. Exact short-circuit order, forwarded type/context, return/current pointer and RET0 checked. All flush/find/allocate children mocked, not accepted by this proof; actual case only80/NULL before next named stub.'),indent=2)+'\n');print(len(rows),'whole original/canonical OMF record dependency/reuse shapes pass')
