"""Instruction symbol-slot visitor: ABI, cached slots and callback mutations."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,re,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@visit_machine_symbols@12';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==4
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='instruction',location='ECX',type='MachineInstruction pointer'),dict(name='visitor',location='EDX',type='MachineSymbolVisitor function pointer'),dict(name='context',location='entryESP+4',type='opaque callback context pointer')]
 assert d['formals'][1]['logical']=='@collect_assembly_reference@8'and d['return_contract']=='void; EAX unconsumed'
 for side in ['original','native']:
  v=d[side];assert [f[side]for f in d['formals']]==[v[k]for k in ['instruction','visitor','context']]and v['initial_head']==0 and v['operation']==1 and v['count']==2
  a,b=v['operands'];assert (a['index'],a['kind'])==(1,4)and a['address']and a['slot']==a['address']+4 and a['symbol']and a['kind']==a['flags']&15
  assert (b['index'],b['kind'],b['address'],b['slot'],b['symbol'])==(0,1,0,0,0)
 assert d['original']['visitor']==0x279a0 and d['original']['context']==d['original']['entry_sp']+12 and d['original']['return_address']==0x2786e and d['original']['instruction']==d['original_parent']['instruction']
 assert d['native']['visitor']==d['native']['callback']and d['native']['context']==d['native']['owner_context']and d['native']['instruction']==d['native']['parent_instruction']and d['native']['caller']=='extract_assembly_labels'
validate(e);reject=[]
for label,edit in [('missing context',lambda d:d['formals'].pop()),('wrong stack slot',lambda d:d['formals'][2].update(location='entryESP+8')),('wrong callback',lambda d:d['native'].update(visitor=0)),('lost context owner',lambda d:d['native'].update(owner_context=0)),('wrong opcode',lambda d:d['original'].update(operation=2)),('wrong indirect slot',lambda d:d['native']['operands'][0].update(slot=0)),('invented hidden input',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=0))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,KeyError,IndexError):reject.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x27920 and row['end']==0x27995 and original.read(0x27992,3)==b'\xc2\x04\0'
ops=re.findall(r'OP\((\w+),\s*(-?\d+),\s*[^)]*\)',Path('src/machine-opcodes.def').read_text());counts=bytes(int(v)&255 for n,v in ops);assert len(counts)==231 and original.read(0x81560,len(counts))==canonical.read(names['_machine_operand_counts'],len(counts))==counts
if args.abi_only:print('Machine visitor3-formal/RET4/operand slot ABI,117B extent,231 named counts and8 negatives pass');raise SystemExit
operation={count:next(i for i,v in enumerate(counts)if v==count)for count in [0,1,2]}

def run(image,side,c):
 instruction,context=0x400000,(0x400080 if c['context']else 0);addresses=[0x400100,0x400200];alternate=0x400300;symbols=[0x400400,0x400500];replacement=0x400600 if c['mode']==1 else 0;callback=0x500000;end=0x501000
 buffers={instruction:bytearray((i*13+5)&255 for i in range(32)),0x400080:bytearray(b'abcd')}
 for a in addresses+[alternate]:buffers[a]=bytearray((i*11+7)&255 for i in range(16))
 struct.pack_into('<I',buffers[instruction],8,0xa5000043|(operation[c['count']]<<8))
 for i,kind in enumerate(c['kinds']):
  address=addresses[0]if c['alias']else addresses[i];pointer=0 if c['null']&(1<<i)else symbols[i];struct.pack_into('<II',buffers[instruction],16+i*8,0xbabe0000|kind,address if 3<=kind<=6 else pointer if kind==8 else 0xdeadbeef)
  if 3<=kind<=6:struct.pack_into('<I',buffers[address],4,pointer)
 expected={a:bytearray(b)for a,b in buffers.items()};events=[]
 def get(a):
  base=next(b for b,v in expected.items()if b<=a and a+4<=b+len(v));return struct.unpack_from('<I',expected[base],a-base)[0]
 def put(a,data):
  base=next(b for b,v in expected.items()if b<=a and a+len(data)<=b+len(v));expected[base][a-base:a-base+len(data)]=data
 for counter in range(c['count'],0,-1):
  i=1 if counter==2 else 0;operand=instruction+16+i*8;kind=get(operand)&15;slot=(get(operand+4)+4)if 3<=kind<=6 else operand+4 if kind==8 else 0
  if not slot or not get(slot):continue
  symbol=get(slot);identifier=0 if c['mode']==0 else 0x12345678 if c['mode']==1 else 0xffffffff;mutations=[]
  if c['mutate']:
   mutations=[(operand,struct.pack('<II',1,alternate)),(instruction+8,struct.pack('<I',0x43))]
   if context:mutations.append((context,struct.pack('<I',0xabc00000+i)))
   if i==1:mutations.append((instruction+16,struct.pack('<II',8,symbols[0])))
  events.append(dict(kind='callback',symbol=symbol,context=context,identifier=identifier,mutations=mutations))
  for a,b in mutations:put(a,b)
  if identifier:events.append(dict(kind='lookup',identifier=identifier,result=replacement));put(slot,struct.pack('<I',replacement))
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x2000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 for a,b in buffers.items():u.mem_write(a,bytes(b))
 u.mem_write(0x300800,struct.pack('<II',end,context));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,instruction);u.reg_write(UC_X86_REG_EDX,callback);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 lookup=names['@lookup_global_symbol@4']if side else 0x4dce0;observed=[]
 def step(m,a,size,_):
  if a==end:m.emu_stop();return
  if a not in [callback,lookup]:return
  ev=events[len(observed)];esp=m.reg_read(UC_X86_REG_ESP)
  if a==callback:
   assert ev['kind']=='callback'and m.reg_read(UC_X86_REG_ECX)==ev['symbol']and m.reg_read(UC_X86_REG_EDX)==ev['context'];result=ev['identifier']
   for p,b in ev['mutations']:m.mem_write(p,b)
  else:assert ev['kind']=='lookup'and m.reg_read(UC_X86_REG_ECX)==ev['identifier'];result=ev['result']
  observed.append(ev['kind']);m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,step);u.emu_start(names[name]if side else row['start'],0,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==end and u.reg_read(UC_X86_REG_ESP)==0x300808 and all(u.reg_read(r)==v for r,v in saved)and len(observed)==len(events)
 for a,b in expected.items():assert bytes(u.mem_read(a,len(b)))==b,(c,hex(a),u.mem_read(a,len(b)).hex(),b.hex())
 return dict(events=observed,expected_output_sha256=hashlib.sha256(b''.join(bytes(expected[a])for a in sorted(expected))).hexdigest())
controls=[]
for left,right in itertools.product(range(16),repeat=2):controls.append(dict(count=2,kinds=[left,right],mode=(left+right)%3,alias=False,null=(left^right)&3,context=bool(left&1),mutate=bool(right&1)))
for count,kinds,mode,alias,mutate in itertools.product([0,1,2],[[4,4],[3,6],[8,8],[1,2]],[0,1,2],[False,True],[False,True]):controls.append(dict(count=count,kinds=kinds,mode=mode,alias=alias,null=0,context=True,mutate=mutate))
rows=[]
for c in controls:
 a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/machine-symbol-visitor-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole117B original/canonical and named231-entry operand count identity. All16x16 operand kind pairs,0/1/2 counts, NULL slots, direct and four indirect classes, aliasing slots, zero/nonzero callback IDs and explicit mocked NULL lookup result. Exact reverse order, opaque context, cached oldslot despite callback operand mutation, fresh later operand reads and cached count despite opcode mutation; every other byte/RET4/nonvolatile preserved. Callback/lookup bodies mocked; real current callback remains named trap and substitution path not claimed executed. EAX unclaimed for void ABI; no new guard/data/child payload snapshots.'),indent=2)+'\n');print(len(rows),'whole machine-symbol visitor controls and8 actual ABI negatives pass')
