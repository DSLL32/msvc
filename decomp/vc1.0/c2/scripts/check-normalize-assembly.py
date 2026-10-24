"""Assembly normalization callback: exact canonical identity and label uses."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@normalize_assembly_flow_symbol@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer'),dict(name='context',location='EDX',type='opaque callback context (unused)')]
 for side in ['original','native']:
  v=d[side];assert v['symbol']==v['parent_symbol']==v['member']==d['formals'][0][side]and v['symbol']and v['parent_slot']and v['scope']and v['kind']==1 and v['identifier']==160 and v['index']==0 and v['context']==v['parent_context']==d['formals'][1][side]==0
 assert d['original']['return_address']==0x2797b and d['native']['caller']=='visit_machine_symbols'and d['return_contract']=='full EAX zero unless canonical label differs, then its identifier'
validate(e);rejected=[]
for label,edit in [('missing context',lambda d:d['formals'].pop()),('wrong context register',lambda d:d['formals'][1].update(location='ECX')),('wrong kind',lambda d:d['native'].update(kind=3)),('unowned symbol',lambda d:d['original'].update(member=0)),('wrong caller slot',lambda d:d['native'].update(parent_symbol=0)),('invented context',lambda d:d['original'].update(context=1)),('unknown hidden operand',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=8))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x5b1d0 and row['end']==0x5b201 and original.read(0x5b200,1)==b'\xc3'
assert original.read(0x5b1de,5)==b'\xe8'+struct.pack('<i',0x5abd0-0x5b1e3)
assert original.read(0x5b1ed,6)==bytes.fromhex('ff402c8b401c')and original.read(0x5b1f7,3)==bytes.fromhex('ff462c')
# These are existing typed fields, not newly copied data or fabricated guards.
source=Path('src/flow-edges.c').read_text();assert 'symbol = label->canonical->label;'in source and '++((LabelSymbol *)symbol)->references;'in source
if args.abi_only:print('Assembly normalization2-formal/RET0/closed symbol ABI,49B extent and8 rejects pass');raise SystemExit

def run(image,side,kind,relation,references,identifier,context,mutation):
 symbol,other,label,redirect=0x400000,0x400100,0x400200,0x400300
 before={a:bytes((i*17+(a>>4)+5)&255 for i in range(n))for a,n in [(symbol,50),(other,50),(label,20),(redirect,20)]};expected={a:bytearray(b)for a,b in before.items()}
 expected[symbol][12]=kind;expected[other][12]=3
 for a in [symbol,other]:struct.pack_into('<I',expected[a],44,references);struct.pack_into('<I',expected[a],28,identifier)
 struct.pack_into('<II',expected[label],0,symbol,label if relation==0 else redirect);struct.pack_into('<II',expected[redirect],0,symbol if relation==2 else other,redirect)
 before={a:bytes(b)for a,b in expected.items()}
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 for a,b in before.items():u.mem_write(a,b)
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,context)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@lookup_flow_label@4']if side else 0x5abd0;events=[];terminal=[]
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a!=child:return
  assert kind==3 and m.reg_read(UC_X86_REG_ECX)==symbol and not events;events.append(['lookup','same symbol']);esp=m.reg_read(UC_X86_REG_ESP)
  if mutation:
   # Callback must use original input pointer for self, and fresh child-owned
   # canonical fields/reference count/identifier after the child has returned.
   expected[symbol][12]=1
   for target in [symbol,other]:struct.pack_into('<I',expected[target],44,(references+7)&0xffffffff);struct.pack_into('<I',expected[target],28,identifier^0x81234567)
   for target in [symbol,other]:u.mem_write(target,bytes(expected[target]))
  m.reg_write(UC_X86_REG_EAX,label);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=1000)
 result=0
 if kind==3:
  target=other if relation==1 else symbol;count=struct.unpack_from('<I',expected[target],44)[0];struct.pack_into('<I',expected[target],44,(count+1)&0xffffffff)
  if relation:result=struct.unpack_from('<I',expected[target],28)[0]
 assert terminal==['return']and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==result and all(u.reg_read(r)==v for r,v in saved)
 assert events==([['lookup','same symbol']]if kind==3 else[])
 for a,b in expected.items():assert bytes(u.mem_read(a,len(b)))==b,(kind,relation,references,identifier,context,mutation,hex(a))
 return dict(result=result,events=events,output_sha256=hashlib.sha256(b''.join(bytes(expected[a])for a in sorted(expected))).hexdigest())
rows=[]
for kind,relation,references,identifier,context,mutation in itertools.product([0,1,3,4,255],[0,1,2],[0,0x7fffffff,0xffffffff],[0,0x81234567],[0,0xdeadbeef],[False,True]):
 a=run(original,False,kind,relation,references,identifier,context,mutation);b=run(canonical,True,kind,relation,references,identifier,context,mutation);assert a==b;rows.append(dict(kind=kind,relation=['self','redirect-other','redirect-same-symbol'][relation],references=references,identifier=identifier,context=context,child_mutation=mutation,result=a))
Path('cases/normalize-assembly-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole49B original/canonical, kind gate0/1/3/4/255, exact sole lookup pointer and no lookup otherwise, self versus distinct canonical entry pointing to other or same original symbol; DWORD+44 increment wraps machine32 and only redirected entry returns full identifierDWORD+28 includingzero/highbit. Child clobbers caller-saved and mutates source kind/reference/id to verify retained original pointer and fresh postlookup values. ContextNULL/arbitrary unmapped opaque pointer never dereferenced. Every other byte/RET0/nonvolatile checked. Child body mocked; first actualkind1 does not exercise kind3 branch. No guard/data/helper or hidden operand added.'),indent=2)+'\n');print(len(rows),'whole normalization controls and8 actual ABI negatives pass')
