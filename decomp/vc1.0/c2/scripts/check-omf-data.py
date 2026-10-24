"""Actual caller-local byte source and whole data callback forwarding ABI."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,cfg,_,_=indexes();names=publics();name='@omf_output_data@8';row=next(r for r in cfg['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());ref=Path(e['original_reference']['directory']);binding=ref/'original-binding.json';assert hashlib.sha256(binding.read_bytes()).hexdigest()==e['original_reference']['binding_sha256'];bound=json.loads(binding.read_text());assert all(hashlib.sha256((ref/n).read_bytes()).hexdigest()==h for n,h in bound['files'].items())
def validate(e):
 assert e['matched']and e['function']==name and e['original_va']=='0x6b6c0'and e['no_hidden_inputs_proof']and e['hidden_inputs']==row['abi']['hidden_inputs']==[]and e['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:f[k]for k in ['name','location','type']}for f in e['formals']]==row['abi']['parameters']==[dict(name='data',location='ECX',type='void pointer'),dict(name='bytes',location='EDX',type='unsigned32')]and e['formals'][1]['value']==4
 a=e['original'];b=e['native'];assert a['source']==e['formals'][0]['original']==a['entry_sp']+4 and a['return_address']==0x69cf6 and b['source']==b['owner']==e['formals'][0]['native']and b['caller']=='write_codeview_signature'and b['owner_size']==4
 shapes=[]
 for d in [a,b]:
  assert d['source_hex']=='01000000'and d['count']==4 and d['current']==d['base']and d['used']==28
  raw=bytes.fromhex(d['record_hex']);assert len(raw)==28 and struct.unpack('<5I',raw[:20])==(0xa1,d['debug_owner'],0,0,d['current']+28);shapes.append(raw[20:])
 assert shapes[0]==shapes[1]
validate(e);reject=[]
for label,change in [('missing count',lambda e:e['formals'].pop()),('wrong data register',lambda e:e['formals'][0].update(location='EDX')),('wrong original local owner',lambda e:e['original'].update(entry_sp=0)),('wrong native local owner',lambda e:e['native'].update(owner=0)),('wrong source bytes',lambda e:e['native'].update(source_hex='00000000')),('wrong current alias',lambda e:e['native'].update(current=0)),('missing record bytes',lambda e:e['native'].update(record_hex='')),('hidden pointer',lambda e:e.update(hidden_inputs=['out?']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):reject.append(label)
 else:raise AssertionError('Accepted '+label)
assert row['start']==0x6b6c0 and row['end']==0x6b6e4 and original.read(0x6b6e3,1)==b'\xc3'

def run(image,native,count,returned,mutate):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x2000),(0x500000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 source=0x400080;data=bytes((i*13+5)&255 for i in range(32));u.mem_write(source,data);pool=names['_omf_records']if native else 0x7a8dc;current=names['_omf_current_record']if native else 0x7b88c;before=bytes((i*17+3)&255 for i in range(4016));u.mem_write(pool,before);u.mem_write(current,struct.pack('<I',pool));u.mem_write(0x300800,struct.pack('<I',0x500000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,source);u.reg_write(UC_X86_REG_EDX,count)
 saved=[(UC_X86_REG_EBX,0x12345678),(UC_X86_REG_ESI,0x23456789),(UC_X86_REG_EDI,0x3456789a),(UC_X86_REG_EBP,0x456789ab)]
 for r,v in saved:u.reg_write(r,v)
 children={names[n]if native else a:k for n,a,k in [('@omf_reserve_bytes@8',0x6b6f0,'reserve'),('@omf_append_bytes@12',0x6b650,'append')]};events=[]
 def hook(m,a,size,_):
  if a==0x500000:m.emu_stop();return
  if a not in children:return
  k=children[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP)
  if k=='reserve':
   assert ecx==pool and edx==count;events.append(['reserve',0,edx]);result=pool+returned*1004;pop=4
   if mutate:m.mem_write(current,struct.pack('<I',pool+1004))
  else:
   assert ecx==pool+returned*1004 and edx==source and struct.unpack('<I',m.mem_read(current,4))[0]==ecx and struct.unpack('<I',m.mem_read(esp+4,4))[0]==count;events.append(['append',returned,'source',count]);result=0xdeadbeef;pop=8
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_ESP,esp+pop);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',m.mem_read(esp,4))[0])
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if native else row['start'],0,count=300)
 assert u.reg_read(UC_X86_REG_EIP)==0x500000 and u.reg_read(UC_X86_REG_ESP)==0x300804 and all(u.reg_read(r)==v for r,v in saved)and bytes(u.mem_read(source,32))==data and bytes(u.mem_read(pool,4016))==before and struct.unpack('<I',u.mem_read(current,4))[0]==pool+returned*1004 and len(events)==2
 return events
rows=[]
for count,returned,mutate in itertools.product([0,4,16,0xffffffff],[0,3],[False,True]):
 a=run(original,False,count,returned,mutate);b=run(canonical,True,count,returned,mutate);assert a==b;rows.append(dict(count=count,returned_slot=returned,child_mutates_current=mutate,events=a))
Path('cases/omf-data-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=reject,states=len(rows),cases=rows,seconds=time.perf_counter()-started,scope='Whole36-byte original/canonical callback: source/count survive reserve call, returned slot overrides child-mutated global, global published before append, full countbits/RET4 append child and RET0 caller ABI/callee-saved verified.16 controls mock both children; countffffffff tests request forwarding only, no huge-copy execution claim. Actual initialized4-byte signature caller-local/current28 used record bytes/aliases observed; raw-register GDB signed-address failure retained, native-only correction reused original.'),indent=2)+'\n');print(len(rows),'whole OMF data callback controls and8 actual ABI rejects pass')
