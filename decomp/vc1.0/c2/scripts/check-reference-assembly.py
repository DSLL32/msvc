"""Assembly flow reference callback: kind gate and opaque work forwarding."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@reference_assembly_flow_symbol@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer'),dict(name='context',location='EDX',type='WorkItem pointer callback context')]
 for side in ['original','native']:
  v=d[side];assert v['symbol']==v['parent_symbol']==v['member']==d['formals'][0][side]and v['symbol']and v['parent_slot']and v['scope']and v['kind']==1 and v['identifier']==160 and v['index']==0 and v['context']==v['parent_context']==d['formals'][1][side]and v['context']
 assert d['original']['return_address']==0x2797b and d['native']['caller']=='visit_machine_symbols'and d['return_contract']=='full EAX zero'
validate(e);rejected=[]
for label,edit in [('missing context',lambda d:d['formals'].pop()),('wrong context register',lambda d:d['formals'][1].update(location='ECX')),('wrong kind',lambda d:d['native'].update(kind=3)),('unowned symbol',lambda d:d['original'].update(member=0)),('wrong caller slot',lambda d:d['native'].update(parent_symbol=0)),('lost work context',lambda d:d['original'].update(context=0)),('unknown hidden operand',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=8))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x5afb0 and row['end']==0x5afcc and original.read(0x5afcb,1)==b'\xc3'
assert original.read(0x5afb9,5)==b'\xe8'+struct.pack('<i',0x5abd0-0x5afbe)
assert original.read(0x5afc3,5)==b'\xe8'+struct.pack('<i',0x5b150-0x5afc8)
assert original.read(0x5afbe,3)==bytes.fromhex('8b5008')
if args.abi_only:print('Assembly reference2-formal/RET0/closed symbol/work ABI,28B extent and8 rejects pass');raise SystemExit

def run(image,side,kind,context,target,mutation):
 symbol,label=0x400000,0x400100
 before={symbol:bytes((i*13+7)&255 for i in range(50)),label:bytes((i*19+5)&255 for i in range(20))};expected={a:bytearray(b)for a,b in before.items()};expected[symbol][12]=kind;struct.pack_into('<I',expected[label],8,target);before={a:bytes(b)for a,b in expected.items()}
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 for a,b in before.items():u.mem_write(a,b)
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,context)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if side else a:k for k,n,a in [('lookup','@lookup_flow_label@4',0x5abd0),('connect','@connect_flow_blocks@8',0x5b150)]};events=[];terminal=[]
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a not in children:return
  child=children[a];esp=m.reg_read(UC_X86_REG_ESP);assert kind==3
  if child=='lookup':
   assert not events and m.reg_read(UC_X86_REG_ECX)==symbol;events.append(['lookup','same symbol']);result=label
   if mutation:
    expected[symbol][12]=1;struct.pack_into('<I',expected[label],8,target^0x81234567)
    for address,b in expected.items():m.mem_write(address,bytes(b))
  else:
   assert events==[['lookup','same symbol']]and m.reg_read(UC_X86_REG_ECX)==context and m.reg_read(UC_X86_REG_EDX)==(target^0x81234567 if mutation else target);events.append(['connect',context,m.reg_read(UC_X86_REG_EDX)]);result=0xdeadbeef
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=1000)
 assert terminal==['return']and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==0 and all(u.reg_read(r)==v for r,v in saved)
 assert len(events)==(2 if kind==3 else 0)
 for a,b in expected.items():assert bytes(u.mem_read(a,len(b)))==b,(kind,context,target,mutation,hex(a))
 return dict(result=0,events=events,output_sha256=hashlib.sha256(b''.join(bytes(expected[a])for a in sorted(expected))).hexdigest())
rows=[]
for kind,context,target,mutation in itertools.product([0,1,3,4,255],[0,0x400800,0xdeadbeef],[0,0x400800,0x400900],[False,True]):
 a=run(original,False,kind,context,target,mutation);b=run(canonical,True,kind,context,target,mutation);assert a==b;rows.append(dict(kind=kind,context=context,target=target,lookup_mutation=mutation,result=a))
Path('cases/reference-assembly-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole28B original/canonical, kind0/1/3/4/255 gate, sole lookup exactsymbol and then connect(context,returnedFlowLabel.work+8), retained opaquecontext across caller-saved clobbers, fresh targetfield after lookupmutations, EAX0 despite childresultpoison, RET0/nonvolatile/all other bytes. Context and work are opaque mocked-child inputs, includingNULL/unmapped/alias values; these controls do not claim real connect accepts invalidWorkItems. Actual firstkind1 skips takenlabelpath; no actualedgeinsertion claim. No new guard/data/hidden input.'),indent=2)+'\n');print(len(rows),'whole assembly-reference controls and8 actual ABI negatives pass')
