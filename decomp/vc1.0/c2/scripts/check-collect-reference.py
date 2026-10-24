"""Assembly-reference callback: kind gate, identity duplicates and fresh head."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@collect_assembly_reference@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer'),dict(name='context',location='EDX',type='callback context pointing to Node * head')]
 for side in ['original','native']:
  v=d[side];assert v['symbol']==v['parent_symbol']==v['member']==d['formals'][0][side]and v['symbol']and v['parent_slot']and v['scope']and v['kind']==1 and v['head']==0 and v['identifier']==160 and v['index']==0 and v['context']==d['formals'][1][side]
 assert d['original']['context']==d['original_parent']['context']and d['original']['return_address']==0x2797b
 assert d['native']['context']==d['native']['parent_context']and d['native']['caller']=='visit_machine_symbols'and d['return_contract']=='full EAX zero'
validate(e);reject=[]
for label,edit in [('missing context',lambda d:d['formals'].pop()),('wrong context register',lambda d:d['formals'][1].update(location='ECX')),('wrong kind',lambda d:d['native'].update(kind=4)),('unowned symbol',lambda d:d['original'].update(member=0)),('wrong caller slot',lambda d:d['native'].update(parent_symbol=0)),('invented old head',lambda d:d['original'].update(head=1)),('unknown hidden operand',lambda d:d.update(hidden_inputs=['?'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=8))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):reject.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x279a0 and row['end']==0x27a1e and original.read(0x27a1d,1)==b'\xc3'
assert original.string(0x80d20)=='PNt'and original.string(struct.unpack('<I',original.read(0x805e8,4))[0])=='@(#)MDmisc.c:1.87'
source=Path('src/inline-assembly.c').read_text();assert '((TemporarySymbol *)symbol)->attributes |= 0x2000;'in source
if args.abi_only:print('Collection callback2-formal/RET0/closed symbol/head ABI,126B extent and8 rejects pass');raise SystemExit

def run(image,side,kind,count,duplicate,returns,mutate):
 symbol,context,leaf,result,alternate=0x400000,0x400100,0x400200,0x400300,0x400400
 if kind in [3,4]and mutate:context=0
 if kind not in [1,3,4]and not returns and mutate:context=0
 before={};expected={};head=0x401000 if count else 0
 for a,n in [(symbol,44),(leaf,28),(result,28),(alternate,28)]:
  b=bytearray((i*17+5)&255 for i in range(n));before[a]=bytes(b);expected[a]=bytearray(b)
 b=bytearray(before[symbol]);b[12]=kind;before[symbol]=bytes(b);expected[symbol]=bytearray(b)
 if context:before[context]=struct.pack('<I',head);expected[context]=bytearray(before[context])
 for i in range(count):
  node=0x401000+i*64;ref=0x402000+i*64;b=bytearray((j*13+i+7)&255 for j in range(28));struct.pack_into('<I',b,12,ref);struct.pack_into('<I',b,16,0 if i==count-1 else node+64);before[node]=bytes(b);expected[node]=bytearray(b);b=bytearray((j*19+3)&255 for j in range(28));struct.pack_into('<I',b,20,symbol if duplicate==i else 0x403000+i*64);before[ref]=bytes(b);expected[ref]=bytearray(b)
 ignored=kind in [3,4];fatal=kind not in [1,3,4];found=0<=duplicate<count;allocate=not ignored and (not fatal or returns)and not found
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 for a,b in before.items():u.mem_write(a,b)
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,context);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if side else a:k for k,n,a in [('error','@compiler_internal_error@8',0x6fb70),('leaf','_intern_leaf',0x4f790),('build','_build_tree',0x4fe10)]};events=[];terminal=[]
 def word(a):return struct.unpack('<I',u.mem_read(a,4))[0]
 def hook(m,a,size,_):
  if a==0x500000:terminal.append('return');m.emu_stop();return
  if a not in children:return
  child=children[a];esp=m.reg_read(UC_X86_REG_ESP);retval=0xdeadbeef
  if child=='error':
   assert fatal and m.reg_read(UC_X86_REG_EDX)==0x4bf and bytes(m.mem_read(m.reg_read(UC_X86_REG_ECX),19)).split(b'\0')[0]==b'@(#)MDmisc.c:1.87';events.append(['error',0x4bf])
   if not returns:terminal.append('fatal-stop');m.emu_stop();return
  elif child=='leaf':assert allocate and [word(esp+i)for i in [4,8,12,16,20]]==[0x26,0,symbol,0,0x404];events.append(['leaf',0x26,0,'same symbol',0,0x404]);retval=leaf
  else:
   assert allocate and bytes(m.mem_read(word(esp+4),4))==b'PNt\0'and [word(esp+i)for i in [8,12,16]]==[leaf,0x55,0x404];events.append(['build','PNt','leaf',0x55,0x404]);retval=result
   if mutate:u.mem_write(context,struct.pack('<I',alternate));expected[context][:]=struct.pack('<I',alternate)
  m.reg_write(UC_X86_REG_EAX,retval);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=10000)
 if fatal and not returns:assert terminal==['fatal-stop']and events==[['error',0x4bf]]
 else:
  assert terminal==['return']and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==0 and all(u.reg_read(r)==v for r,v in saved)
  assert [v[0]for v in events]==(['error']if fatal else[])+(['leaf','build']if allocate else[])
  if allocate:struct.pack_into('<I',expected[result],16,alternate if mutate else head);expected[context][:]=struct.pack('<I',result)
 for a,b in expected.items():assert bytes(u.mem_read(a,len(b)))==b,(kind,count,duplicate,returns,mutate,hex(a))
 return dict(terminal=terminal[0],events=events,allocated=allocate,output_sha256=hashlib.sha256(b''.join(bytes(expected[a])for a in sorted(expected))).hexdigest())
rows=[]
for kind,count,duplicate,returns,mutate in itertools.product([1,3,4,0,2,255],[0,1,3],[-1,0,2],[False,True],[False,True]):
 a=run(original,False,kind,count,duplicate,returns,mutate);b=run(canonical,True,kind,count,duplicate,returns,mutate);assert a==b;rows.append(dict(kind=kind,count=count,duplicate_index=duplicate,diagnostic_returns=returns,builder_changes_head=mutate,result=a))
Path('cases/collect-assembly-reference-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole126B original/canonical: kind3/4 skip evenNULLcontext, kind1 lists empty/1/3 and exact symbol-pointer duplicate positions, invalidkind exact4bf/source diagnostic with stop versus explicitly mocked return, no real failure-path claim. Exact cdecl varargs of intern_leaf/build_tree PNt, fresh head reload after builder mutation, sole new second-link/head writes, EAX0/RET0/nonvolatile. No child body/payload copy or arbitrary invalid-context continuation claim. Parent kind1 attribute access refined to existing TemporarySymbol.attributes at sameDWORD+36; no code extent/data credit for that type refinement.'),indent=2)+'\n');print(len(rows),'whole collection-callback controls and8 actual ABI negatives pass')
