"""Actual no-formal ABI and whole signed/reloaded pending-fixup traversal."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument('--abi-only',action='store_true');args=ap.parse_args();started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_omf_write_fixups';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6c400'and e['formals']==row['abi']['parameters']==[]and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['no_hidden_inputs_proof']
 assert e['original']['count']==e['native']['count']==4 and e['original']['pool']==0x79154 and e['native']['pool']and e['original']['return_address']==0x6c3eb and e['native']['caller']=='omf_flush_record'
validate(e);reject=[]
for label,change in [('invented ECX formal',lambda e:e.update(formals=[dict(name='entry',location='ECX')])),('wrong native count',lambda e:e['native'].update(count=1)),('wrong original count',lambda e:e['original'].update(count=-1)),('wrong pool',lambda e:e['original'].update(pool=0)),('missing native pool',lambda e:e['native'].update(pool=0)),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('invented entry fields',lambda e:e.update(hidden_inputs=['pool_fields']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c400 and row['end']==0x6c437 and original.read(0x6c436,1)==b'\xc3'
assert original.read(0x6c41e,3)==bytes.fromhex('83c618')and original.read(0x6c422,8)==bytes.fromhex('3b3d945008007ced')
assert 'extern OmfFixup omf_fixups[250];'in Path('src/omf-output.h').read_text()and len({names['_omf_fixups'],names['_omf_fixup_count'],names['_omf_fixup_threads']})==3
if args.abi_only:print('Fixup writer actual no-formal/count4/pool ABI/7 rejects and55-byte stride24/reloaded-signed-bound extent pass');raise SystemExit

def run(image,native,count,mode,poison):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in[(0x10000,0x100000),(0x300000,0x10000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_fixups']if native else 0x79154;counter=names['_omf_fixup_count']if native else 0x85094;child=names['@omf_write_fixup@4']if native else 0x6c680;u.mem_write(counter,struct.pack('<I',count&0xffffffff));u.mem_write(0x300800,struct.pack('<2I',0x500000,0xfeedbeef));u.reg_write(UC_X86_REG_ESP,0x300800)
 for r in[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX]:u.reg_write(r,poison)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 terminal=False;events=[]
 def access(m,kind,address,size,value,_):assert address+size<=pool or address>=pool+6000,'Writer must not read or write entry fields'
 def hook(m,a,size,_):
  nonlocal terminal
  if a==0x500000:terminal=True;m.emu_stop();return
  if a!=child:return
  pointer=m.reg_read(UC_X86_REG_ECX);assert pointer==pool+24*len(events)and pointer<pool+6000;events.append(len(events))
  if len(events)==1 and mode!='unchanged':
   bound={'stop':0,'one':1,'negative':-1,'extend':min(count+2,250)}[mode];m.mem_write(counter,struct.pack('<I',bound&0xffffffff))
  esp=m.reg_read(UC_X86_REG_ESP);m.reg_write(UC_X86_REG_EAX,0xdeadbeef);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access);u.emu_start(names[name]if native else row['start'],0,count=20000)
 expected=0 if count<=0 else (count if mode=='unchanged'else min(count+2,250)if mode=='extend'else 1)
 assert terminal and events==list(range(expected))and struct.unpack('<I',u.mem_read(counter,4))[0]==0 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(0x300804,4))==struct.pack('<I',0xfeedbeef)
 return dict(slots=events,final_count=0,stack_cleanup=0)
rows=[]
for count,mode,poison in itertools.product([0,-1,-0x80000000,1,4,249,250],['unchanged','stop','one','negative','extend'],[0,0x89abcdef,0xffffffff]):
 a=run(original,False,count,mode,poison);b=run(canonical,True,count,mode,poison);assert a==b;rows.append(dict(count=count,child_count_mutation=mode,incoming_caller_registers=poison,result=a))
Path('cases/omf-fixup-writer-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole55-byte writer, sole signed counter and fixed typed pool, no formals.105 original/canonical states cover zero/negative/no-iteration reset and1/4/249/250 entries, first child changes bound to0/1/-1/original+2<=250, incoming caller-register poison, stride24, RET0/saved regs/stack guard. Memory hooks reject any pool-field read/write: entry pointers are child arguments, no opaque input snapshot or seeded old entry data. Serializer child mocked and not accepted by these controls. Positive count>250 outside fixed pool lifecycle is not valid success input; no invented target guard. Actual count4/pool/caller observed separately; full exact OMF output remains pending.'),indent=2)+'\n');print(len(rows),'whole fixup writer controls/7 actual ABI rejects pass')
