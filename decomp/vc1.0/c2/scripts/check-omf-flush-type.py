"""Actual OMF cache/type inputs and whole first-match flush lookup controls."""
from pathlib import Path
import copy,hashlib,itertools,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();name='@omf_flush_record_type@4';row=next(r for r in config['driver']if r['name']==name);p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());binding=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in binding['files'].items())
def validate(r):
 assert r['matched']and r['function']==name and r['no_hidden_inputs_proof']and r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert [{k:a[k]for k in ['name','location','type']}for a in r['formals']]==row['abi']['parameters']==[dict(name='type',location='ECX',type='unsigned32')]
 assert [a['value']for a in r['formals']]==r['original_values']==r['native']['values']==[0x95]
 pool=r['pool_input']['original'];other=r['pool_input']['native'];assert pool['stride']==1004 and pool['count']==4 and pool['current']==pool['base']and other['current']==other['base']
 assert pool['headers']==other['headers']==[bytes(20).hex()]*4
validate(e);rejected=[]
for label,change in [('missing type',lambda r:r.update(formals=[])),('wrong register',lambda r:r['formals'][0].update(location='EDX')),('wrong pool stride',lambda r:r['pool_input']['original'].update(stride=1000)),('wrong current alias',lambda r:r['pool_input']['native'].update(current=0)),('changed defined record header',lambda r:r['pool_input']['native']['headers'].__setitem__(0,'01'+bytes(19).hex())),('unknown hidden pointer',lambda r:r.update(hidden_inputs=['out?']))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except(AssertionError,KeyError):rejected.append(label)
 else:raise AssertionError('Accepted '+label)
def run(image,side,type,matches,change):
 u=Uc(UC_ARCH_X86,UC_MODE_32)
 for a,n in [(0x10000,0x100000),(0x300000,0x10000),(0x400000,0x1000)]:u.mem_map(a,n)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_records']if side else 0x7a8dc;before=bytearray((i*37+9)&255 for i in range(4016))
 for i in range(4):struct.pack_into('<I',before,1004*i,type if i in matches else type^1)
 u.mem_write(pool,bytes(before));u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);u.reg_write(UC_X86_REG_ECX,type);flush=names['@omf_flush_record@4']if side else 0x6c330;events=[]
 def hook(m,a,size,unused):
  if a==0x400000:m.emu_stop();return
  if a!=flush:return
  pointer=m.reg_read(UC_X86_REG_ECX);index=(pointer-pool)//1004;assert pointer==pool+1004*index and index==min(matches);events.append(index);esp=m.reg_read(UC_X86_REG_ESP)
  if change:
   u.mem_write(pointer,struct.pack('<I',type^1));before[index*1004:index*1004+4]=struct.pack('<I',type^1)
  m.reg_write(UC_X86_REG_EAX,0x12345678);m.reg_write(UC_X86_REG_ECX,0xabcdef01);m.reg_write(UC_X86_REG_EDX,0xabcdef02);m.reg_write(UC_X86_REG_EIP,struct.unpack('<I',u.mem_read(esp,4))[0]);m.reg_write(UC_X86_REG_ESP,esp+4)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(names[name]if side else row['start'],0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804 and u.reg_read(UC_X86_REG_EAX)==bool(matches)and bytes(u.mem_read(pool,4016))==before
 return events
rows=[]
for type,matches,change in itertools.product([0,0x95,0xffffffff],[(),(0,),(1,),(2,),(3,),(0,3),(0,1,2,3)],[0,1]):
 a=run(original,False,type,matches,change);b=run(canonical,True,type,matches,change);assert a==b;rows.append(dict(type=hex(type),matches=matches,child_changes_tag=change,selected=a))
Path('cases/omf-flush-type-validation.json').write_text(json.dumps(dict(actual_abi=True,rejected=rejected,cases=rows,seconds=time.perf_counter()-started,scope='Whole37-byte original/canonical first matching type across four1004-byte buffers.42 states cover no match/every slot/duplicates/type0 andFFFFFFFF; exact0/1 return independent of mocked child pointer result, tag mutation/short-circuit and unchanged other pool bytes. Child flush body mocked and remains named trap. Actual first95 observes four fully zero20-byte headers/current slot0; no payload input or arbitrary reuse-state claim.'),indent=2)+'\n');print(len(rows),'whole OMF first-type flush states and6 actual ABI/pool rejects pass')
