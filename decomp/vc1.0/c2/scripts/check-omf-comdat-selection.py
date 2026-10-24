"""Unsigned-byte COMDAT selection ABI, all inputs and original diagnostic edge."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_comdat_selection@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6c2f0'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='selection',location='CL',type='unsigned8')]
 for side in ['original','native']:
  d=e[side];assert d['value']==d['owner_selection']==e['formals'][0][side]==2 and d['segment']
 assert e['original']['ecx']&255==2 and e['original']['return_address']==0x6c18c and e['native']['caller']=='omf_initialize_record'and e['return_contract']=='unsigned8 inAL, upperEAX unspecified/unconsumed'
validate(e);reject=[]
for label,change in [('missing formal',lambda e:e.update(formals=[])),('wrong register width',lambda e:e['formals'][0].update(location='ECX')),('wrong value',lambda e:e['native'].update(value=1)),('wrong caller byte',lambda e:e['native'].update(owner_selection=1)),('unproved fullEAX',lambda e:e.update(return_contract='unsigned32 EAX')),('invented context formal',lambda e:e.update(hidden_inputs=['segment']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c2f0 and row['end']==0x6c329 and original.read(0x6c328,1)==b'\xc3'
assert original.read(0x6c184,8)==b'\x8a\x4f\x44\xe8'+struct.pack('<i',0x6c2f0-0x6c18c)
if args.abi_only:print(name,'actual CL/AL ABI/6 negatives/57B extent/caller-byte checks pass; full256-input controls explicit');raise SystemExit

def run(image,native,value,upper,returns):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,upper|value);u.reg_write(UC_X86_REG_EAX,0xdeadbeef)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 child=names['@compiler_internal_error@8']if native else 0x6fb70;events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a!=child:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);text=bytes(m.mem_read(ecx,18)).split(b'\0')[0];assert text==b'@(#)omf.c:1.10'and edx==0x541;events.append(['diagnostic',text.decode(),edx])
  if not returns:m.emu_stop();return
  esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,0xabcdef12);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=1000);expected={1:0x10,2:0,3:0x20,4:0x30};fatal=value not in expected
 assert len(events)==int(fatal)
 if fatal and not returns:assert u.reg_read(UC_X86_REG_EIP)==child;result=None
 else:assert all(u.reg_read(r)==v for r,v in saved)and u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804;result=u.reg_read(UC_X86_REG_EAX)&255;assert result==expected.get(value,0)
 return dict(events=events,result_AL=result,returned=not fatal or returns)
rows=[]
for value,upper,returns in itertools.product(range(256),[0,0xa5a50000,0xffffff00],[False,True]):
 a=run(original,False,value,upper,returns);b=run(canonical,True,value,upper,returns);assert a==b;rows.append(dict(value=value,upper_ecx=upper,diagnostic_returns=returns,result=a))
Path('cases/omf-comdat-selection-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole57B original/canonical, all256 lowbyte inputs x3 upperECX poisons x stop/explicit mocked-return diagnostic.1/2/3/4 ->10/0/20/30; all others exactomf.c:1.10/code541, and explicit postcallAL0 when mocked diagnostic returns. FullEAX unspecified for unsignedchar result; callee-saved/RET0 checked. Actual selection2 only, error branch not executed by real case and compiler_internal_error remains named trap. No new data/ABI guard.'),indent=2)+'\n');print(len(rows),'whole COMDAT selection controls and6 actual ABI rejects pass')
