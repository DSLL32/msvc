"""Lexical S_BLOCK32 ABI, initialized record bytes and callback/global ordering."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@write_codeview_block@12';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
params=[dict(name=k,location=l,type=t)for k,l,t in [('symbol','ECX','FunctionSymbol pointer'),('offset','EDX','unsigned32'),('bytes','entryESP+4','unsigned32')]]
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x69b20'and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==4 and e['no_hidden_inputs_proof']
 assert [{k:f[k]for k in['name','location','type']}for f in e['formals']]==row['abi']['parameters']==params
 shape=dict(offset=20,bytes=65,function_start=0,scope_start=20,scope_end=85,debug_offset=122);assert e['shape']==shape and e['original']['return_address']==0x695d5 and e['native']['caller']=='write_debug_scopes'and e['native']['scope']==e['native']['caller_scope']
 for side in ['original','native']:
  d=e[side];assert all(d[k]==v for k,v in shape.items())and d['symbol']==d['caller_symbol']and d['symbol']and d['scope']and d['debug']and[f[side]for f in e['formals']]==[d[k]for k in['symbol','offset','bytes']]
validate(e);reject=[]
for label,change in [('missing bytes',lambda e:e['formals'].pop()),('wrong byte-count location',lambda e:e['formals'][2].update(location='EAX')),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=0)),('wrong caller symbol',lambda e:e['original'].update(caller_symbol=0)),('wrong scope alias',lambda e:e['native'].update(caller_scope=0)),('wrong scope start',lambda e:e['native'].update(scope_start=21)),('wrong function start',lambda e:e['original'].update(function_start=1)),('missing debug segment',lambda e:e['native'].update(debug=0)),('wrong mutable debug offset',lambda e:e['original'].update(debug_offset=0)),('invented stack-local record',lambda e:e.update(hidden_inputs=['record']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x69b20 and row['end']==0x69b97 and original.read(0x69b94,3)==bytes.fromhex('c20400')and original.read(0x69b24,7)==bytes.fromhex('66c74424041500')
assert 'sizeof(CodeViewBlockRecord) == 24'in Path('src/codeview-output.c').read_text()
if args.abi_only:print('CodeView block actual3 formals/lexical owner/debug word/10 rejects and119-byte RET4/packed layout pass');raise SystemExit

def run(image,native,start,offset,byte_count,debug_offset,mode):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;old=0x400900;new=0x400a00;global_pointer=names['_debug_symbols_segment']if native else 0x7914c;callbacks=names['_output_callbacks']if native else 0x7b8f0;expected=struct.pack('<HHIIIIHB',21,0x207,0,0,byte_count,(offset-start)&0xffffffff,0,0);assert len(expected)==23
 d=bytearray((i*13+5)&255 for i in range(96));struct.pack_into('<I',d,4,start);u.mem_write(symbol,bytes(d));a=bytearray((i*29+7)&255 for i in range(96));b=bytearray(a);struct.pack_into('<I',a,4,debug_offset);struct.pack_into('<I',b,4,0x89abcdef);u.mem_write(old,bytes(a));u.mem_write(new,bytes(b));u.mem_write(global_pointer,struct.pack('<I',old))
 for delta,child in[(40,0x500100),(36,0x500200),(16,0x500300)]:u.mem_write(callbacks+delta,struct.pack('<I',child))
 u.mem_write(0x300800,struct.pack('<3I',0x500000,byte_count,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EDX,offset);saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 events=[];terminal=False;current=old
 def hook(m,address,size,_):
  nonlocal terminal,current
  if address==0x500000:terminal=True;m.emu_stop();return
  if address not in[0x500100,0x500200,0x500300]:return
  esp=m.reg_read(UC_X86_REG_ESP);ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX)
  if address==0x500100:
   assert not events and ecx==old;kind='select';struct.pack_into('<I',d,4,(start+123)&0xffffffff);m.mem_write(symbol,bytes(d))
  elif address==0x500200:
   assert len(events)==1 and edx==23 and bytes(m.mem_read(ecx,23))==expected;kind='data'
  else:assert len(events)==2 and ecx==0xfffffff9 and edx==symbol;kind='size'
  events.append(kind)
  if mode==kind:current=new;m.mem_write(global_pointer,struct.pack('<I',new))
  m.reg_write(UC_X86_REG_EAX,0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=600);target=a if current==old else b;prior=struct.unpack_from('<I',target,4)[0];struct.pack_into('<I',target,4,(prior+23)&0xffffffff)
 assert terminal and events==['select','data','size']and u.reg_read(UC_X86_REG_ESP)==0x300808 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300808,4))==struct.pack('<I',0xfeedbeef)and bytes(u.mem_read(symbol,96))==d and bytes(u.mem_read(old,96))==a and bytes(u.mem_read(new,96))==b and struct.unpack('<I',u.mem_read(global_pointer,4))[0]==current
 return dict(events=events,record_hex=expected.hex(),updated_owner='old'if current==old else'new',updated_offset=(prior+23)&0xffffffff)
rows=[]
for pair,byte_count,debug_offset,mode in itertools.product([(0,20),(20,0),(0xffffffff,0),(0x80000000,0x7fffffff),(20,20)],[0,65,0x80000000,0xffffffff],[0,122,0xffffffff],['unchanged','select','data','size']):
 start,offset=pair;a=run(original,False,start,offset,byte_count,debug_offset,mode);b=run(canonical,True,start,offset,byte_count,debug_offset,mode);assert a==b;rows.append(dict(function_start=start,offset=offset,bytes=byte_count,debug_offset=debug_offset,child_global_swap=mode,result=a))
Path('cases/codeview-block-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole119-byte writer versus independent packed23-byte S_BLOCK32 <HHIIIIHB model, final structure alignment byte not emitted/initialized/observed.240 original/canonical states cover offset subtraction unsigned wrap,scalar byte-count patterns,debug count wrap; select child changes symbol start proving record was cached before children, global debug-segment swaps at each callback prove final reload/update owner. Exact select/data23/size-7 symbol requests, RET4/saved-register/stack and all owned guards. Children mocked with explicit state changes; no cold callback behavior inferred. Actual symbol/offset20/bytes65/functionstart0/scope20..85/debugoffset122 observed separately, no stack-local record input or irrelevant source tails.'),indent=2)+'\n');print(len(rows),'whole CodeView block controls/10 actual ABI rejects pass')
