"""Whole zero-extension expansion with independent ISA plans and ABI evidence."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,re,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@expand_zero_extension@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
expected=dict(index=19,opcode=312128,destination_flags=1,destination_register=1,source_flags=2309,source_value=None,following=dict(opcode=508480),following_operands=None,address=dict(displacement=0,base=27,index=1,symbol_kind=1,attributes=66086,local=0,symbol_offset=0),address_symbol_present=True)
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x24740'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='instruction',location='ECX',type='MachineInstruction pointer')]and e['shape']==expected
 for side in ['original','native']:
  d=e[side];assert d['instruction']==e['formals'][0][side]==d['next_previous']and d['instruction']and d['next']and d['source_value']==d['address']['pointer']and d['address']['symbol']
  for k in ['index','opcode','destination_flags','destination_register','source_flags','following','following_operands']:assert d[k]==expected[k]
  assert {k:v for k,v in d['address'].items()if k not in['pointer','symbol']}==expected['address']
 assert e['original']['return_address']==0x2457c and e['native']['caller']=='finalize_machine_context'
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EAX')),('broken link alias',lambda e:e['native'].update(next_previous=0)),('wrong opcode',lambda e:e['native'].update(opcode=0)),('wrong index register',lambda e:e['native']['address'].update(index=2)),('wrong source identity',lambda e:e['native']['address'].update(pointer=0)),('wrong following',lambda e:e['native']['following'].update(opcode=0)),('invented allocation input',lambda e:e.update(hidden_inputs=['new_instruction']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x24740 and row['end']==0x24bfd and original.read(0x24bfc,1)==b'\xc3'
assert original.read(0x24577,5)==b'\xe8'+struct.pack('<i',0x24740-0x2457c)
ops=re.findall(r'^OP\((\w+)',Path('src/machine-opcodes.def').read_text(),re.M);assert{n:ops[n]for n in[1,0x2e,0x34,0xa9,0xc2,0xc3]}=={1:'MOV',0x2e:'AND',0x34:'XOR',0xa9:'NOP',0xc2:'MOVZX',0xc3:'MOVZXW'}
if args.abi_only:print(name,'actual ABI/8 rejects/1213B extent/ISA identities pass; full plan controls explicit');raise SystemExit
MOV,AND,XOR,NOP,MOVZX,MOVZXW=1,0x2e,0x34,0xa9,0xc2,0xc3
HIGH=0x100000

def plan(c):
 low=(c['destination']&0x3fff)<=3;fold=c['following']==2;reg=c['source'].startswith('r');aligned=False;same=False
 if reg:
  n=int(c['source'][1:])&0x3fff;n=n-4 if n>=4 else n
  if c['operation']==MOVZXW:return(True,MOV,4,AND,4,0xffff)
  if c['width']==2:return(not(c['high']and not low),MOV,1 if low else 2,AND,4,0xffff00ff)
  if low and n!=(c['destination']&0x3fff)and not fold:return(True,XOR,4,MOV,1,0)
  return(not(c['high']and not low),MOV,1 if c['high']else 4,AND,4,0xff)
 same=c['source']=='indexed' or c['source']=='based-same'
 local=c['source']in['local-aligned','local-unaligned'];offset=0 if c['source']=='local-aligned'else 1
 aligned=local and ((offset+int(c['high']))&(c['width']-1))==0
 if c['operation']==MOVZX:
  if c['width']==2:return(low or aligned,MOV,1 if low else 2,AND,4,0xffff00ff)
  if low and not same and not fold:return(True,XOR,4,MOV,1,0)
  return(low or aligned,MOV,1 if low else 4,AND,4,0xff)
 if not same and not fold and not aligned:return(True,XOR,4,MOV,2,0)
 return(True,MOV,4 if aligned else 2,AND,4,0xffff)

def run(image,native,c):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x3000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 inst,following,after,inserted,before,address,symbol=0x400100,0x400200,0x400300,0x400400,0x400500,0x401000,0x401100
 records={a:bytearray((i*19+(a>>8))&255 for i in range(32))for a in[inst,following,after,inserted,before]}
 def put(a,o,v):struct.pack_into('<I',records[a],o,v&0xffffffff)
 def get(a,o):return struct.unpack_from('<I',records[a],o)[0]
 opcode=0x24000000|(int(c['high'])<<20)|(c['operation']<<8)|(c['width']<<4);put(inst,0,following);put(inst,4,before);put(inst,8,opcode);put(inst,16,0x901);put(inst,20,0xabcd0000|c['destination']);put(following,0,after);put(following,4,inst);put(following,8,(AND if c['following']else NOP)<<8|0x40);put(following,16,0xa01);put(following,20,c['destination']+(1 if c['following']==1 else 0));put(following,24,1 if c['following']==3 else 2);put(following,28,0x12345678);put(after,4,following);put(before,0,inst);put(inserted,8,0x25100000)
 reg=c['source'].startswith('r');kind=1 if reg else 5 if c['source']=='indexed'else 4
 put(inst,24,0x900|kind);put(inst,28,0xdcba0000|int(c['source'][1:])if reg else address)
 mem=bytearray((i*31+9)&255 for i in range(16));sym=bytearray((i*17+5)&255 for i in range(44));sym[12]=1;struct.pack_into('<I',sym,36,0x10 if c['source'].startswith('local-')else 0x20);struct.pack_into('<I',sym,4,0)
 struct.pack_into('<4I',mem,0,0 if c['source']!='local-unaligned'else 1,symbol,c['destination']if c['source']=='based-same'else 27,c['destination']if c['source']=='indexed'else 28)
 for a,b in records.items():u.mem_write(a,bytes(b))
 u.mem_write(address,bytes(mem));u.mem_write(symbol,bytes(sym));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,inst)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 specs=[('equal','@operands_equal@8',0x219d0),('new','@new_machine_instruction@4',0x172f0),('operand','@initialize_operand@20',0x19710),('remove','@remove_machine_instruction@4',0x1dd60)];children={names[n]if native else a:k for k,n,a in specs};events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',m.mem_read(esp,4))[0];result=0xdeadbeef;cleanup=0
  if k=='equal':assert ecx==inst+16 and edx==following+16;result=int(c['following']!=1);events.append(['equal',result])
  elif k=='new':assert ecx==0;result=inserted;events.append(['new'])
  elif k=='operand':
   number,displacement,symarg=struct.unpack('<3I',m.mem_read(esp+4,12));number&=65535;assert symarg==0 and edx in[1,2]and ecx in[inserted+16,inserted+24,inst+24];m.mem_write(ecx,struct.pack('<2I',edx,number if edx==1 else displacement));cleanup=12;events.append(['operand',ecx-inst,edx,number,displacement])
  else:
   assert ecx==following;prev,next=struct.unpack('<2I',m.mem_read(following,8))[1],struct.unpack('<I',m.mem_read(following,4))[0];assert prev==inserted and next==after;m.mem_write(prev,struct.pack('<I',next));m.mem_write(next+4,struct.pack('<I',prev));m.mem_write(following,bytes(4));events.append(['remove'])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4+cleanup);m.reg_write(UC_X86_REG_EIP,ret)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=3000)
 expand,op1,w1,op2,w2,mask=plan(c)
 if not reg and c['high']:put(inst,8,get(inst,8)&~HIGH);struct.pack_into('<I',mem,0,(struct.unpack_from('<I',mem)[0]+1)&0xffffffff)
 if expand:
  put(inst,8,(get(inst,8)&~0xfff0)|(op1<<8)|(w1<<4));put(inserted,8,(get(inserted,8)&~0xfff0)|(op2<<8)|(w2<<4));put(inserted,16,1);put(inserted,20,c['destination'])
  if op1==XOR:
   put(inserted,24,get(inst,24));put(inserted,28,get(inst,28));put(inserted,8,(get(inserted,8)&~HIGH)|(get(inst,8)&HIGH));put(inst,8,get(inst,8)&~HIGH);put(inst,24,1);put(inst,28,c['destination'])
  else:put(inserted,24,2);put(inserted,28,mask)
  put(inst,0,inserted);put(inserted,0,following);put(inserted,4,inst);put(following,4,inserted)
  if c['following']==2:put(inserted,28,0x12345678&mask);put(inserted,0,after);put(after,4,inserted);put(following,0,0)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_EAX)==int(expand)and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and all(bytes(u.mem_read(a,32))==b for a,b in records.items())and bytes(u.mem_read(address,16))==mem and bytes(u.mem_read(symbol,44))==sym,(c,plan(c),events,[(hex(a),bytes(u.mem_read(a,32)).hex(),b.hex())for a,b in records.items()if bytes(u.mem_read(a,32))!=b])
 return dict(plan=list(plan(c)),events=events,state_sha256=hashlib.sha256(b''.join(bytes(b)for b in records.values())+mem+sym).hexdigest())
rows=[]
for operation,width,destination,source,high,following in itertools.product([MOVZX,MOVZXW],[2,4],[1,6,0x4001],['r0','r1','r5','based-same','indexed','local-aligned','local-unaligned','foreign'],[False,True],range(4)):
 c=dict(operation=operation,width=width,destination=destination,source=source,high=high,following=following);a=run(original,False,c);b=run(canonical,True,c);assert a==b;rows.append(dict(inputs=c,result=a))
Path('cases/zero-extension-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole1213B original/canonical expansion vs independent ISA plan. MOVZX/MOVZXW,width2/4, byte/nonbyte/tagged destination, register/highbyte/addressalias/localalignment cases, followingAND equality/kind, even refused memory highbyte adjustment, exact operation/width/mask, child args+order, all5x32 record/16address/44symbol bytes/callee-saved/RET0/EAX checked. Four children mocked; no child semantic attribution. Actual indexedglobal MOVZXW4 dest1 observed, cold variants are bounded model controls rather than compiler branch coverage. No new target data.'),indent=2)+'\n');print(len(rows),'whole zero-extension plans and8 actual ABI rejects pass')
