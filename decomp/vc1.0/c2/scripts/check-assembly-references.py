"""Whole assembly-reference extraction with exact child ABI/list ownership."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@extract_assembly_labels@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='node',location='ECX',type='Node pointer')]
 for side in ['original','native']:
  v=d[side];assert v['node']==v['parent_node']==d['formals'][0][side]and v['instruction']==v['parent_instruction']and v['node']and v['instruction']
 assert d['original']['return_address']==0x52051 and d['native']['caller']=='rewrite_before_children'and d['return_contract']=='EAX Node pointer orNULL'
validate(e);reject=[]
for label,edit in [('missing formal',lambda d:d.update(formals=[])),('wrong register',lambda d:d['formals'][0].update(location='EDX')),('lost node identity',lambda d:d['native'].update(parent_node=0)),('wrong forwarded pointer',lambda d:d['original'].update(instruction=0)),('invented output input',lambda d:d.update(hidden_inputs=['head?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4)),('wrong result',lambda d:d.update(return_contract='boolean'))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):reject.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x27850 and row['end']==0x27912 and original.read(0x27911,1)==b'\xc3'
assert original.string(0x80d18)=='NPNt'and original.read(0x27861,5)==b'\xba'+struct.pack('<I',0x279a0)
callback=next(r for r in cfg['driver']if r['start']==0x279a0);assert callback['name']=='@collect_assembly_reference@8'
if callback['name']not in cfg['accepted']:assert callback.get('provisional')and callback['end']==0x279a1
else:assert not callback.get('provisional')and callback['end']==0x27a1e and callback['abi']['entry_observation']=='cases/collect-assembly-reference-abi/validation.json'
source=Path('src/inline-assembly.c').read_text();assert 'visit_machine_symbols(node->left.pointer, collect_assembly_reference, &head);'in source and 'Node *head = NULL, *tail, *result;'in source
if args.abi_only:print('Assembly-reference actual pointer/forwarded field/caller ABI,194B extent/callback identity and7 rejects pass');raise SystemExit

def run(image,side,count,flags,word,alias):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x20000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 node,instruction,sentinel,integer,result=0x400000,0x400100,0x401000,0x402000,0x403000
 raw=bytearray((i*17+3)&255 for i in range(28));struct.pack_into('<I',raw,12,instruction);u.mem_write(node,bytes(raw));before={};expected={};chain=[];symbols=[]
 for i in range(count):
  n=0x404000+i*64;leaf=0x405000+i*64;symbol=0x406000+(0 if alias else i)*128;chain.append(n);symbols.append(symbol);b=bytearray((j*13+i*3+7)&255 for j in range(28));struct.pack_into('<I',b,0,0x55);struct.pack_into('<I',b,12,leaf);struct.pack_into('<I',b,16,0 if i==count-1 else n+64);before[n]=bytes(b);expected[n]=bytearray(b);u.mem_write(n,bytes(b));b=bytearray((j*19+5)&255 for j in range(28));struct.pack_into('<I',b,20,symbol);before[leaf]=bytes(b);expected[leaf]=bytearray(b);u.mem_write(leaf,bytes(b));b=bytearray((j*11+9)&255 for j in range(84));struct.pack_into('<H',b,14,word);struct.pack_into('<I',b,36,flags);before[symbol]=bytes(b);expected[symbol]=bytearray(b);u.mem_write(symbol,bytes(b))
 for a in [sentinel,integer,result]:b=bytearray((j*23+1)&255 for j in range(28));struct.pack_into('<I',b,0,0x4c if a==sentinel else 0x40);before[a]=bytes(b);expected[a]=bytearray(b);u.mem_write(a,bytes(b))
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,node);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if side else a:k for k,n,a in [('visit','@visit_machine_symbols@12',0x27920),('leaf','_intern_leaf',0x4f790),('integer','@integer_node@8',0x58430),('build','_build_tree',0x4fe10)]};events=[]
 def read32(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  kind=children[a];esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);cleanup=0
  if kind=='visit':
   assert ecx==instruction and edx==(names[callback['name']]if side else 0x279a0);out=read32(esp+4);assert 0x300000<=out<0x310000 and read32(out)==0;u.mem_write(out,struct.pack('<I',chain[0]if count else 0));events.append(['visit','same instruction','named callback','local NULL head output']);retval=0xdeadbeef;cleanup=4
  elif kind=='leaf':
   assert count and read32(esp+4)==0x4c and read32(esp+8)==0 and read32(chain[-1]+16)==0;events.append(['leaf',0x4c,0]);retval=sentinel
  elif kind=='integer':
   assert ecx==0xf0 and edx==0x102 and read32(chain[-1]+16)==sentinel;events.append(['integer',ecx,edx]);retval=integer
  else:
   fmt=read32(esp+4);text=bytes(u.mem_read(fmt,5));assert text==b'NPNt\0'and [read32(esp+i)for i in [8,12,16,20]]==[0x4c,integer,0x40,0x102];events.append(['build','NPNt',0x4c,'integer',0x40,0x102]);retval=result
  m.reg_write(UC_X86_REG_EAX,retval);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,read32(esp));m.reg_write(UC_X86_REG_ESP,esp+4+cleanup)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(node,28))==raw
 if count:
  assert u.reg_read(UC_X86_REG_EAX)==result and [v[0]for v in events]==['visit','leaf','integer','build'];struct.pack_into('<I',expected[chain[-1]],16,sentinel);struct.pack_into('<I',expected[result],20,chain[0])
  for symbol in symbols:struct.pack_into('<I',expected[symbol],36,flags|0x2000);struct.pack_into('<H',expected[symbol],14,word|0x2000)
 else:assert u.reg_read(UC_X86_REG_EAX)==0 and len(events)==1
 for a,b in expected.items():assert bytes(u.mem_read(a,len(b)))==b,(hex(a),u.mem_read(a,len(b)).hex(),b.hex())
 return dict(events=events,result='wrapper'if count else'NULL',memory_sha256=hashlib.sha256(b''.join(bytes(expected[a])for a in sorted(expected))).hexdigest())
rows=[]
for count,flags,word,alias in itertools.product([0,1,2,5],[0,0x2000,0xffffffff],[0,0x2000,0xffff],[False,True]):
 a=run(original,False,count,flags,word,alias);b=run(canonical,True,count,flags,word,alias);assert a==b;rows.append(dict(count=count,flags=flags,word=word,duplicate_symbol_alias=alias,result=a))
Path('cases/assembly-references-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole194B original/canonical: sole forwarded instruction pointer, callback identity and local head initiallyNULL; empty and1/2/5-node output lists, exact tail sentinel link, intern_leaf cdecl args/typed integer fastcall/outer build_tree varargs NPNt, returned wrapper right=head, every node/symbol byte except exact DWORD+36/WORD+14 OR2000 remains unchanged. Duplicate symbol aliases/idempotence and already-set bits, RET0/nonvolatile exact. Child bodies mocked; their acceptance and real-case transformation execution are tracked by separate fresh frontier receipts.'),indent=2)+'\n');print(len(rows),'whole assembly-reference controls and7 actual ABI negatives pass')
