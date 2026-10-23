"""Current record global input and whole11-byte tail-transfer ABI proof."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='_omf_flush_records';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0 and e['formals']==row['abi']['parameters']==[]and e['native']['values']==e['original_values']==[]
 for r in e['record_input'].values():
  if not isinstance(r,dict):continue
  assert r['pointer']==r['current']==r['base']and r['bytes']==34
  raw=bytes.fromhex(r['hex']);assert len(raw)==34 and struct.unpack('<5I',raw[:20])==(128,0,0,11,r['pointer']+34)and raw[20:]==b'\x80\0\0\x0aomftypes.c'
validate(e);reject=[]
for label,change in [('invented formal',lambda e:e.update(formals=[dict(name='x')])),('hidden register',lambda e:e.update(hidden_inputs=['EAX'])),('wrong cleanup',lambda e:e.update(callee_stack_cleanup=4)),('wrong current identity',lambda e:e['record_input']['native'].update(current=0)),('missing payload',lambda e:e['record_input']['native'].update(hex=bytes(20).hex())),('wrong payload',lambda e:e['record_input']['native'].update(hex=e['record_input']['native']['hex'][:-2]+'ff'))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6a7f0 and row['end']==0x6a7fb

def run(image,native,slot,returned,mutate):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_records']if native else 0x7a8dc;current=names['_omf_current_record']if native else 0x7b88c;child=names['@omf_flush_record@4']if native else 0x6c330;before=bytearray((i*7+3)&255 for i in range(4016));u.mem_write(pool,bytes(before));u.mem_write(current,struct.pack('<I',pool+slot*1004));u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);events=[]
 def hook(m,a,size,_):
  if a==0x400000:m.emu_stop();return
  if a!=child:return
  assert m.reg_read(UC_X86_REG_ECX)==pool+slot*1004;events.append(slot);esp=m.reg_read(UC_X86_REG_ESP)
  if mutate:u.mem_write(pool+slot*1004,bytes(4));before[slot*1004:slot*1004+4]=bytes(4)
  m.reg_write(UC_X86_REG_EAX,pool+returned*1004);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x23456789);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==0x400000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==pool+returned*1004 and bytes(u.mem_read(pool,4016))==before and struct.unpack('<I',u.mem_read(current,4))[0]==pool+slot*1004 and events==[slot]
 return events
rows=[]
for slot,returned,mutate in itertools.product(range(4),range(4),[False,True]):
 assert run(original,False,slot,returned,mutate)==run(canonical,True,slot,returned,mutate);rows.append(dict(current=slot,returned=returned,child_mutates_tag=mutate))
Path('cases/omf-flush-wrapper-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,cases=rows,seconds=time.perf_counter()-start,scope='Whole11-byte original tail transfer vs canonical C call/return.32 controls forward exact current slot/child EAX result and retain every pool/current byte except explicit child mutation. Actual defined34-byte record input observed separately; no child body or completed artifact acceptance.'),indent=2)+'\n');print(len(rows),'whole OMF flush wrapper controls and6 ABI rejects pass')
