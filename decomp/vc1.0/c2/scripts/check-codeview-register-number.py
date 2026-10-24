"""AX-only CodeView register conversion, signed size and diagnostic edge."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@codeview_register_number@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x69900'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='symbol',location='ECX',type='SymbolHeader pointer')]
 for side in ['original','native']:
  d=e[side];assert d['symbol']==d['parent_symbol']==e['formals'][0][side]and d['symbol']and d['size']==4 and d['register_number']==1
 assert e['original']['return_address']==0x698ca and e['native']['caller']=='write_codeview_register'and e['return_contract']=='unsigned16 inAX, caller storesword atrecord+6'
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong ECX',lambda e:e['formals'][0].update(location='EDX')),('wrong caller alias',lambda e:e['native'].update(parent_symbol=0)),('wrong signedsize',lambda e:e['original'].update(size=5)),('wrong registerWORD',lambda e:e['native'].update(register_number=2)),('unproved fullEAX',lambda e:e.update(return_contract='unsigned32 EAX')),('invented parentrecord',lambda e:e.update(hidden_inputs=['parentrecord']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x69900 and row['end']==0x6992e and original.read(0x6992d,1)==b'\xc3'
assert original.read(0x698c5,5)==b'\xe8'+struct.pack('<i',0x69900-0x698ca)
if args.abi_only:print(name,'actual AX ABI/7 rejects/46B extent/caller identity pass; full arithmetic controls explicit');raise SystemExit

def run(image,native,size,register,mode):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 symbol=0x400080;d=bytearray((i*19+5)&255 for i in range(44));struct.pack_into('<i',d,8,size);struct.pack_into('<H',d,40,register);u.mem_write(symbol,bytes(d));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,symbol);u.reg_write(UC_X86_REG_EAX,0xdeadbeef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@compiler_internal_error@8']if native else 0x6fb70;events=[]
 def hook(m,a,n,_):
  if a==0x500000:m.emu_stop();return
  if a!=child:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);text=b''
  for i in range(32):
   b=bytes(m.mem_read(ecx+i,1))
   if b==b'\0':break
   text+=b
  assert text==b'@(#)emit.c:1.20'and edx==0x2fb;events.append(['diagnostic',text.decode(),edx])
  if mode=='stop':m.emu_stop();return
  if mode=='mutate':struct.pack_into('<i',d,8,-2147483648);struct.pack_into('<H',d,40,0x1234);m.mem_write(symbol,bytes(d))
  esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,0xabcdef12);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000);fatal=size>4;assert len(events)==int(fatal)and bytes(u.mem_read(symbol,44))==d
 if fatal and mode=='stop':assert u.reg_read(UC_X86_REG_EIP)==child;result=None
 else:
  result=u.reg_read(UC_X86_REG_EAX)&65535;reg=0x1234 if fatal and mode=='mutate'else register;assert result==(((size//2)*8+reg+1)&65535)and u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 return dict(events=events,result_AX=result,descriptor_sha256=hashlib.sha256(d).hexdigest())
rows=[]
for size,register,mode in itertools.product([-2147483648,-3,-1,0,1,2,3,4,5,0x7fffffff],[0,1,7,65535],['stop','return','mutate']):
 a=run(original,False,size,register,mode);b=run(canonical,True,size,register,mode);assert a==b;rows.append(dict(size=size,register=register,diagnostic_mode=mode,result=a))
Path('cases/codeview-register-number-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole46B original/canonical versus independent arithmeticfloor(size/2)*8+register+1 modulo16.120 cases include actual1/2/4 size families, negative/odd/upper/error sizes, registerWORD boundaries, exactemit2fb diagnostic stop/mockreturn/mutation. Size cached beforechild, registerWORD loaded afterchild, all44B inputstorage/AXonly/callee-savedafterreturn/RET0. Actualsize4/reg1 only; cold invalidsizes and returningdiagnostic are explicit controls, not validproducer/realerrorflow claims. No invented size/registerguard or fullEAX result.'),indent=2)+'\n');print(len(rows),'whole register-number arithmetic controls/7 actual ABI rejects pass')
