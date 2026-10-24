"""Scope constructor: owned48-byte record, fresh child results and list append."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_allocate_exception_scope';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['formals']==row['abi']['parameters']==[]and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and d['return_contract']=='EAX newly allocated ExceptionScope pointer'
 for side in ['original','native']:
  v=d[side];assert v['head']==v['next_identifier']==v['finally_depth']==0 and v['opcode']==0x8f and v['expression']and v['tree']
 assert d['original']['return_address']==0x65cbd and d['native']['caller']=='collect_exception_scopes'and d['native']['tree']==d['native']['parent_tree']
 assert d['original_request']==dict(arena=1,bytes=48,return_address=0x661d1)
 for phase,v in d['original_records'].items():assert v['pointer']+48==v['next']and v['block']+4<=v['pointer']and v['next']<=v['block']+1024 and v['remaining']==v['block']+1024-v['next']and len(bytes.fromhex(v['record_hex']))==48
 assert d['original_records']['before_query']['record_hex']=='00'*48
 assert d['native_owned_record']['record_hex']==d['original_records']['before_query']['record_hex']and d['native_owned_record']['request']==[1,48]
 v=d['native_owned_record'];assert v['pointer']+48==v['next']and v['block']+4<=v['pointer']and v['next']<=v['block']+1024 and v['remaining']==v['block']+1024-v['next']
validate(e);rejected=[]
for label,edit in [('invented formal',lambda d:d.update(formals=[dict(name='function',location='ECX')])),('hidden register',lambda d:d.update(hidden_inputs=['ESI'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4)),('wrong parent caller',lambda d:d['native'].update(caller='unknown')),('wrong allocation arena',lambda d:d['original_request'].update(arena=0)),('short allocation',lambda d:d['original_request'].update(bytes=36)),('wrong actual node',lambda d:d['native'].update(opcode=0x90)),('lost node alias',lambda d:d['native'].update(parent_tree=0)),('native record not cleared',lambda d:d['native_owned_record'].update(record_hex='01'*48)),('native allocation not owned',lambda d:d['native_owned_record'].update(next=0)),('invented return',lambda d:d.update(return_contract='EAX zero'))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x661c0 and row['end']==0x66232 and original.read(0x6622f,3)==bytes.fromhex('5f5ec3')
source=Path('src/flow-edges.h').read_text();assert 'sizeof(ExceptionScope) == 48'in source and 'unsigned kind, state, identifier, finally_depth;'in source and 'struct ExceptionLocalSet *locals;'in source
for n,a in [('_enclosing_try_scope',0x66380),('@push_exception_scope@4',0x65ef0)]:
 r=next(r for r in cfg['driver']if r['name']==n);assert r['start']==a
 if n not in cfg['accepted']:assert r['provisional']and r['end']==a+1
if args.abi_only:print('Scope allocator no-formal/owned48B/RET0 ABI,114B extent and11 rejects pass');raise SystemExit

def run(image,side,identifier,depth,parent_id,list_count,mutation):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 scope=0x401000;parent=0x402000;head=0x403000;alternate=0x404000
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def write(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
 globals=[names[n]for n in ['_prepared_exception_scopes','_next_exception_scope_identifier','_exception_finally_depth']]if side else [0x7b998,0x7b9a4,0x7b9a0];head_cell,id_cell,depth_cell=globals
 u.mem_write(scope-16,b'\xa5'*80);write(parent+12,parent_id or 0);write(head_cell,head if list_count else 0);write(id_cell,identifier);write(depth_cell,depth)
 for i in range(list_count):write(head+64*i,head+64*(i+1)if i+1<list_count else 0)
 write(alternate,0)
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800)
 for r in [UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX]:u.reg_write(r,0xdeadbeef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if side else a:k for k,n,a in [('allocate','@arena_alloc@8',0x70ae0),('parent','_enclosing_try_scope',0x66380),('push','@push_exception_scope@4',0x65ef0)]};events=[];terminal=[]
 expected=bytearray(48);struct.pack_into('<I',expected,12,identifier);struct.pack_into('<I',expected,16,depth)
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a not in children:return
  kind=children[a];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);value=0xdeadbeef
  if kind=='allocate':assert ecx==1 and edx==48;events.append([kind,ecx,edx]);value=scope
  elif kind=='parent':
   assert bytes(m.mem_read(scope,48))==expected and word(id_cell)==(identifier+1)&0xffffffff;events.append([kind,bytes(m.mem_read(scope,48)).hex()]);value=0 if parent_id is None else parent
   if mutation=='parent-state':write(id_cell,0x76543210);write(depth_cell,0x12345678)
  else:
   assert ecx==scope;struct.pack_into('<I',expected,20,0xffffffff if parent_id is None else parent_id);assert bytes(m.mem_read(scope,48))==expected;events.append([kind,ecx])
   if mutation=='replace-head':write(head_cell,alternate)
   elif mutation=='replace-tail'and list_count:write(head+64*(list_count-1),alternate)
   elif mutation=='scope-state':write(scope+44,0x98765432);struct.pack_into('<I',expected,44,0x98765432)
  m.reg_write(UC_X86_REG_EAX,value);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=10000)
 assert terminal==['return']and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==scope and all(u.reg_read(r)==v for r,v in saved)
 assert bytes(u.mem_read(scope,48))==expected and bytes(u.mem_read(scope-16,16))==bytes(u.mem_read(scope+48,16))==b'\xa5'*16
 actual_head=word(head_cell);cursor=actual_head;chain=[]
 while cursor:assert cursor not in chain and len(chain)<8;chain.append(cursor);cursor=word(cursor)
 expected_chain=[alternate,scope]if mutation=='replace-head'else [head+64*i for i in range(list_count)]+([alternate]if mutation=='replace-tail'and list_count else [])+[scope]
 assert chain==expected_chain and len(events)==3
 return dict(events=events,record=expected.hex(),chain=chain,identifier=word(id_cell),finally_depth=word(depth_cell),adjacent_untouched=True,return_pointer=scope)
rows=[]
for identifier,depth,parent_id,list_count in itertools.product([0,0xffffffff,0x80000000],[0,3],[None,0,0xfedcba98],[0,1,3]):
 a=run(original,False,identifier,depth,parent_id,list_count,'none');b=run(canonical,True,identifier,depth,parent_id,list_count,'none');assert a==b;rows.append(dict(identifier=identifier,depth=depth,parent_id=parent_id,list_count=list_count,mutation='none',result=a))
for mutation in ['parent-state','replace-head','replace-tail','scope-state']:
 for list_count in [0,1,3]:
  a=run(original,False,7,4,13,list_count,mutation);b=run(canonical,True,7,4,13,list_count,mutation);assert a==b;rows.append(dict(identifier=7,depth=4,parent_id=13,list_count=list_count,mutation=mutation,result=a))
Path('cases/scope-allocate-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole114B original/canonical constructor with mocked children. Exact arena1/48 request, twelve-word clear with adjacent poisons, id wrapping/depth/parent identifier, NULL -1 parent, fresh list head/tail after push, child-mutated globals/record, EAX pointer/RET0/nonvolatile. Actual native execution currently reaches only parent-query boundary; full-return/list controls are independent synthetic model evidence, not native real-case return claim.'),indent=2)+'\n');print(len(rows),'whole constructor controls and11 actual ABI rejects pass')
