"""Actual OMF source-name ABI and whole THEADR-wrapper forwarding controls."""
from pathlib import Path
import copy,hashlib,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='@omf_write_module_header@4';row=next(r for r in config['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(r):
 assert r['matched']and r['function']==name and r['original_va']=='0x6a980'and r['no_hidden_inputs_proof']
 assert r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in r['formals']]==row['abi']['parameters']==[dict(name='source',location='ECX',type='char pointer')]
 a=r['formals'][0];assert a['original']==r['original_source_global']and a['native']==r['native']['source_global']==r['native']['pointer']and a['original']and a['native']
 assert bytes.fromhex(r['original_string_hex'])==bytes.fromhex(r['native']['hex'])==b'omftypes.c\0'
 assert {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(e);rejected=[]
for label,change in [('missing source formal',lambda r:r.update(formals=[])),('wrong source register',lambda r:r['formals'][0].update(location='EDX')),('broken source ownership',lambda r:r['native'].update(source_global=0)),('changed source string',lambda r:r['native'].update(hex='00')),('unknown hidden pointer',lambda r:r.update(hidden_inputs=['out?']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
def run(image,side,pointer):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x200000,0x10000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 word=lambda a:struct.unpack('<I',u.mem_read(a,4))[0];u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,pointer)
 calls={names[n]if side else a:k for k,n,a in [('begin','@omf_begin_record@8',0x6b480),('append','@omf_append_string@8',0x6c4e0),('flush','_omf_flush_records',0x6a7f0)]};events=[]
 def hook(m,a,size,unused):
  if a==0x400000:m.emu_stop();return
  if a not in calls:return
  k=calls[a];ecx=m.reg_read(UC_X86_REG_ECX);edx=m.reg_read(UC_X86_REG_EDX);esp=m.reg_read(UC_X86_REG_ESP);result=0
  if k=='begin':assert ecx==0x80 and edx==0;events.append(['begin',ecx,edx]);result=0x206000
  elif k=='append':assert ecx==0x206000 and edx==pointer;events.append(['append',ecx,edx])
  else:events.append(['flush'])
  m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,word(esp));m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
 return events
rows=[]
for pointer in [0,0x205000,0x205004,0x205100]:
 a=run(original,False,pointer);b=run(canonical,True,pointer);assert a==b;rows.append(dict(pointer=pointer,events=a))
Path('cases/omf-header-validation.json').write_text(json.dumps(dict(actual_source_abi=True,rejected=rejected,controls=rows,seconds=time.perf_counter()-started,scope='Actual ECX char pointer aliases source_filename and full omftypes.c NUL bytes match. Whole31-byte original/canonical wrapper controls verify THEADR80/contextNULL request, returned record/source forwarding and flush order with clobbered caller registers. Children mocked, including NULL source forwarding only; no actual NULL/long-string child semantics or OMF output completion claimed. RET0 stack shape checked.'),indent=2)+'\n');print('Actual OMF source formal/ownership,5 rejects and4 whole wrapper controls pass')
