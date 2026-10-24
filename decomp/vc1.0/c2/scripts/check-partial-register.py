"""Tagged-word partial-write predicate ABI and exact two-lookup control flow."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@register_partial_write_safe@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(d):
 assert d['matched']and d['function']==name and d['no_hidden_inputs_proof']and d['hidden_inputs']==row['abi']['hidden_inputs']==[]and d['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in d['formals']]==row['abi']['parameters']==[dict(name='reg',location='CX',type='unsigned16')]
 assert d['formals'][0]['original']==d['formals'][0]['native']==d['original_raw_ecx']&65535==0x8024
 for side in ['original','native']:assert d[side]['reg']==d[side]['parent_reg']==0x8024
 assert d['original']['return_address']==0x235fb and d['native']['caller']=='overwrites_first_register'and d['return_contract']=='full EAX boolean0/1'
validate(e);rejected=[]
for label,edit in [('missing formal',lambda d:d.update(formals=[])),('wrong input width',lambda d:d['formals'][0].update(location='ECX')),('changed tagged input',lambda d:d['native'].update(reg=0x24)),('lost caller alias',lambda d:d['original'].update(parent_reg=0)),('invented hidden input',lambda d:d.update(hidden_inputs=['ESI'])),('wrong cleanup',lambda d:d.update(callee_stack_cleanup=4)),('wrong return width',lambda d:d.update(return_contract='AL only'))]:
 d=copy.deepcopy(e);edit(d)
 try:validate(d)
 except(AssertionError,IndexError,KeyError):rejected.append(label)
 else:raise AssertionError('accepted '+label)
assert row['start']==0x2db00 and row['end']==0x2db2c and original.read(0x2db2b,1)==b'\xc3'
assert original.read(0x2db06,7)==b'\x8b\xce\xe8'+struct.pack('<i',0x2c870-0x2db0d)
assert original.read(0x2db1a,7)==b'\x8b\xce\xe8'+struct.pack('<i',0x2c870-0x2db21)
# The accepted child masks upper EAX/ESI and compares only CX, so the
# original parent's partial SI assignment introduces no hidden high-word input.
assert original.read(0x2c87b,8).hex()=='25ffff000083e080'
assert original.read(0x2c883,3)==b'\x83\xe6\x1f'and original.read(0x2c891,4)==b'\x66\x39\x48\x18'
if args.abi_only:print('Partial-write actual CX/RET0/boolean ABI,44B extent and7 rejects pass');raise SystemExit

def run(image,side,reg,upper,esi_upper,present,flags):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 first,second=0x400100,0x400200;before=bytes((i*17+3)&255 for i in range(52));other=bytearray(before);struct.pack_into('<I',other,28,flags);u.mem_write(first,before);u.mem_write(second,bytes(other));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,upper|reg)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,esi_upper|0x5678),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@lookup_register_variable@8']if side else 0x2c870;events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a!=child:return
  assert m.reg_read(UC_X86_REG_ECX)&65535==reg and m.reg_read(UC_X86_REG_EDX)==len(events);events.append([reg,len(events)]);esp=m.reg_read(UC_X86_REG_ESP);result=(first if present else 0)if len(events)==1 else second;m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=1000)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)
 assert bytes(u.mem_read(first,52))==before and bytes(u.mem_read(second,52))==other
 result=u.reg_read(UC_X86_REG_EAX);assert result==int(present and bool(flags&0x40))and events==([[reg,0],[reg,1]]if present else[[reg,0]])
 return dict(result=result,calls=events)
rows=[]
for reg,upper,esi_upper,present,flags in itertools.product([0,7,0x4000,0x8024,0xffff],[0,0xdead0000],[0,0xbeef0000],[False,True],[0,0x40,0x80,0xffffffff]):
 a=run(original,False,reg,upper,esi_upper,present,flags);b=run(canonical,True,reg,upper,esi_upper,present,flags);assert a==b;rows.append(dict(reg=reg,upper_ecx=upper,incoming_esi_upper=esi_upper,first_present=present,second_flags=flags,result=a))
Path('cases/partial-register-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole44B original/canonical predicate: optional lookup0 miss returns0 without second call; hit performs required lookup1 and reads its own flags DWORD+28 bit40, not the first record. Tagged CX/upper ECX/upper incoming ESI, exact child order/EDX, full boolean EAX, RET0/nonvolatile and unchanged records checked. Child bodies mocked. Required lookup returningNULL after a returning fatal diagnostic is not claimed supported; original dereferences it too. No new data/guard.'),indent=2)+'\n');print(len(rows),'whole partial-register controls and7 actual ABI negatives pass')
