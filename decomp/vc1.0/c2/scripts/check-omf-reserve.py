"""Actual record/count ABI and signed strict-capacity/renewed-result controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_reserve_bytes@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert e['formals']==[dict(name='record',location='ECX',type='omf_record_current32',value=0),dict(name='bytes',location='EDX',type='unsigned32',value=4)]and row['abi']['parameters']==[dict(name='record',location='ECX',type='OmfRecord pointer'),dict(name='bytes',location='EDX',type='unsigned32')]
 assert e['native']['values']==e['original_values']==[0,4]and len(e['record_pointer_formals'])==1
 f=e['record_pointer_formals'][0];assert f['name']=='record'and f['slot']==0 and f['defined_pointee']=='actual28-byte used record'
 shapes=[]
 for side in ['original','native']:
  d=e['record_input'][side];assert d['pointer']==d['current']==d['base']==f[side+'_pointer']and d['bytes']==28 and d['debug_owner']
  raw=bytes.fromhex(d['hex']);assert len(raw)==28 and struct.unpack('<5I',raw[:20])==(161,d['debug_owner'],0,0,d['pointer']+28);shapes.append(raw[20:])
 assert shapes[0]==shapes[1]==b'\xa1\x0b\0\x05'+bytes(4)
validate(e);reject=[]
for label,change in [('missing bytecount',lambda e:e['formals'].pop()),('wrong count',lambda e:e['native'].update(values=[0,5])),('wrong record owner',lambda e:e['record_input']['native'].update(current=0)),('wrong debug owner',lambda e:e['record_input']['original'].update(debug_owner=0)),('missing defined descriptor',lambda e:e['record_input']['native'].update(hex='')),('invented hidden arg',lambda e:e.update(hidden_inputs=['EAX']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b6f0 and row['end']==0x6b70d and original.read(0x6b70c,1)==b'\xc3'

def signed(v):return v-(1<<32)if v&0x80000000 else v
if args.abi_only:
 print(name,'actual ABI/negative/extent/storage checks pass; full semantic grid is explicit');raise SystemExit

def run(image,native,offset,count,returned):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 record=0x400080;context=0x400800;tag=0xc3 if returned else 0xa1;buf=bytearray((i*17+3)&255 for i in range(1004));struct.pack_into('<5I',buf,0,tag,context,0,71,record+offset);u.mem_write(record,bytes(buf));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,record);u.reg_write(UC_X86_REG_EDX,count);child=names['@omf_initialize_record@12']if native else 0x6c060;events=[]
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a!=child:return
  ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);assert ecx==record and edx==tag and struct.unpack('<I',m.mem_read(esp+4,4))[0]==context;events.append(['initialize',tag,'context']);m.reg_write(UC_X86_REG_EAX,record+returned*1004);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+8);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=300)
 renew=998-offset<=signed(count);assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==record+(returned*1004 if renew else 0)and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(record,1004))==buf and len(events)==int(renew)
 return dict(renew=renew,returned_offset=returned*1004 if renew else 0,events=events)
rows=[]
for offset,count,returned in itertools.product([28,997,998,999,1004],[0,1,4,0x80000000,0xffffffff],[0,1]):
 a=run(original,False,offset,count,returned);b=run(canonical,True,offset,count,returned);assert a==b;rows.append(dict(cursor_offset=offset,count=count,child_returned_other=bool(returned),result=a))
Path('cases/omf-reserve-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole29-byte original/canonical helper: signed32 capacity and count, strict greater preserves/equality renews, zero/negative remaining/count, exact type/context/RET4 initializer arguments and child EAX forwarding versus original record, callee-saved/RET0 and all1004 record bytes unchanged.50 controls mock initializer; out-of-capacity cursor values are arithmetic-only descriptors, no payload dereference/copy or frontend-validity claim. Actual970remaining/count4 preserves record and is independently observed with owned28-byte prefix/context alias.'),indent=2)+'\n');print(len(rows),'whole OMF reserve controls and6 actual ABI rejects pass')
