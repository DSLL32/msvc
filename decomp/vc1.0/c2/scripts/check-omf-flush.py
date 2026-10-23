"""Actual record pointer ABI and whole dependency/flush ordering controls."""
from pathlib import Path
import argparse,copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
ap=argparse.ArgumentParser();ap.add_argument("--abi-only",action="store_true");args=ap.parse_args()
start=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_flush_record@4';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in e['formals']]==row['abi']['parameters']==[dict(name='record',location='ECX',type='omf_record_current32')]and e['formals'][0]['value']==e['native']['values'][0]==e['original_values'][0]==0
 f=e['record_pointer_formals'];assert len(f)==1 and f[0]['name']=='record'and f[0]['slot']==0 and f[0]['defined_pointee']
 for r,k in [('original','original_pointer'),('native','native_pointer')]:
  v=e['record_input'][r];assert v['pointer']==v['current']==v['base']==f[0][k]and v['bytes']==34;raw=bytes.fromhex(v['hex']);assert len(raw)==34 and struct.unpack('<5I',raw[:20])==(128,0,0,11,v['pointer']+34)and raw[20:]==b'\x80\0\0\x0aomftypes.c'
validate(e);reject=[]
for label,change in [('missing pointer',lambda e:e.update(formals=[])),('wrong register',lambda e:e['formals'][0].update(location='EDX')),('unknown hidden input',lambda e:e.update(hidden_inputs=['?'])),('wrong current identity',lambda e:e['record_input']['native'].update(current=0)),('lost pointer owner',lambda e:e['record_pointer_formals'][0].update(native_pointer=0)),('wrong payload',lambda e:e['record_input']['original'].update(hex=bytes(34).hex()))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6c330 and row['end']==0x6c3f5 and original.read(0x6c3f4,1)==b'\xc3'
if args.abi_only:print("OMF record flush actual ABI/6 rejects and197-byte extent pass");raise SystemExit
def run(image,native,type,bits,fixups,change):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_records']if native else 0x7a8dc;current=names['_omf_current_record']if native else 0x7b88c;fix=names['_omf_fixup_count']if native else 0x85094
 children={names[n]if native else a:k for k,n,a in [('type','@omf_flush_record_type@4',0x6aaf0),('write','@omf_write_record@4',0x6c440),('fixups','_omf_write_fixups',0x6c400),('current','_omf_flush_records',0x6a7f0)]};before=bytearray((i*31+9)&255 for i in range(4016));struct.pack_into('<I',before,0,type);u.mem_write(pool,bytes(before));u.mem_write(current,struct.pack('<I',pool));u.mem_write(fix,struct.pack('<I',fixups&0xffffffff));u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,pool);events=[];type_calls=0
 def hook(m,a,size,_):
  nonlocal type_calls
  if a==0x400000:m.emu_stop();return
  if a not in children:return
  kind=children[a];esp=m.reg_read(UC_X86_REG_ESP);tag=struct.unpack('<I',m.mem_read(pool,4))[0];result=0xdeadbeef
  if kind=='type':
   t=m.reg_read(UC_X86_REG_ECX);r=bool(bits>>type_calls&1);type_calls+=1;events.append([kind,t,tag]);result=int(r)
  elif kind=='write':
   assert m.reg_read(UC_X86_REG_ECX)==pool;events.append([kind,tag]);m.mem_write(pool,struct.pack('<I',0xdd));m.mem_write(pool+40,b'\xfe');before[40]=0xfe
   if change:m.mem_write(fix,struct.pack('<I',1 if not fixups else 0))
  elif kind=='fixups':
   events.append([kind,tag]);m.mem_write(current,struct.pack('<I',pool+1004));m.mem_write(fix,bytes(4))
  else:
   events.append([kind,tag,(struct.unpack('<I',m.mem_read(current,4))[0]-pool)//1004])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0x12345678);m.reg_write(UC_X86_REG_EDX,0x23456789);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=10000);assert u.reg_read(UC_X86_REG_EIP)==0x400000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==pool
 expected=[];used=0
 def flush(t):
  nonlocal used
  r=bool(bits>>used&1);used+=1;expected.append(['type',t,type]);return r
 if type:
  if type in [0xa1,0xa3,0xc3]:
   if not flush(0xb0)and not flush(0x8c)and not flush(0xb4):flush(0xbc)
   flush(0x91);flush(0xb7)
  if type==0xbc and not flush(0x96):flush(0xca)
  expected.append(['write',type]);struct.pack_into('<I',before,0,0)
  f=(1 if not fixups else 0)if change else fixups
  if type in [0xa1,0xc3]and f:expected.extend([['fixups',0],['current',0,1]])
 assert events==expected,(events,expected)
 assert bytes(u.mem_read(pool,4016))==before
 return events
rows=[]
for type,bits,fixups,change in itertools.product([0,0x80,0xa1,0xa3,0xc3,0xbc,0xffffffff],range(8),[0,1,-1],[False,True]):
 a=run(original,False,type,bits,fixups,change);b=run(canonical,True,type,bits,fixups,change);assert a==b;rows.append(dict(type=type,return_bits=bits,fixups=fixups,writer_changes_fixups=change,events=a))
Path('cases/omf-flush-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-start,scope='Whole197-byte original/canonical flusher: all dependency classes, flush short-circuit results, empty record, cached old type despite writer tag mutation, post-write fixup count changes/negative nonzero, tag clearing before fixups and global-current change through child, original pointer return/ESP. All child bodies explicitly mocked; actual type80 path only reaches writer. Actual34-byte input alias/payload proved separately.'),indent=2)+'\n');print(len(rows),'whole OMF record flush controls and6 ABI rejects pass')
