"""Four typed OMF wire buffers: exact tag-only reset and current-record identity."""
from pathlib import Path
import copy,hashlib,json,struct,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP
from pe_indexes import indexes,publics
started=time.perf_counter();original,canonical,config,left,right=indexes();names=publics();row=next(r for r in config['driver']if r['name']=='_omf_initialize_records');p=Path(row['abi']['entry_observation']);e=json.loads(p.read_text());b=json.loads(p.with_name('original-binding.json').read_text());assert all(hashlib.sha256((p.parent/n).read_bytes()).hexdigest()==h for n,h in b['files'].items())
def validate(r):
 assert r['matched']and r['function']==row['name']and r['no_hidden_inputs_proof']and r['formals']==row['abi']['parameters']==[]and r['hidden_inputs']==row['abi']['hidden_inputs']==[]and r['callee_stack_cleanup']==row['abi']['callee_stack_cleanup']==0
 assert r['original_values']==r['native']['values']==[]and {k:r['native'][k]for k in ['column','cw','sw']}=={k:r['original_state'][k]for k in ['column','cw','sw']}
validate(e);rejected=[]
for label,change in [('invented formal',lambda r:r.update(formals=['unknown'])),('unknown hidden pointer',lambda r:r.update(hidden_inputs=['out?'])),('missing original proof',lambda r:r.update(no_hidden_inputs_proof=''))]:
 bad=copy.deepcopy(e);change(bad)
 try:validate(bad)
 except AssertionError:rejected.append(label)
 else:raise AssertionError('Accepted '+label)
for image,pool,current in [(original,0x7a8dc,0x7b88c),(canonical,names['_omf_records'],names['_omf_current_record'])]:
 for a,n in [(pool,4*1004),(current,4)]:assert any(s['flags']&0x80 and image.base+s['rva']<=a and a+n<=image.base+s['rva']+s['virtual_size']for s in image.sections)
def run(image,side,poison):
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x100000);u.mem_map(0x300000,0x10000);u.mem_map(0x400000,0x1000)
 for s in image.sections:
  if s['size']and not s['flags']&0x80:u.mem_write(image.base+s['rva'],image.data[s['off']:s['off']+s['size']])
 pool=names['_omf_records']if side else 0x7a8dc;current=names['_omf_current_record']if side else 0x7b88c
 data=bytes(((i*37+poison)&255)for i in range(4*1004));u.mem_write(pool,data);u.mem_write(current,struct.pack('<I',0x12345678));u.mem_write(0x300800,struct.pack('<I',0x400000));u.reg_write(UC_X86_REG_ESP,0x300800);writes=[]
 def store(m,access,a,size,value,unused):
  if pool<=a<pool+4*1004 or a==current:writes.append((a-pool if a!=current else'current',size,value if a!=current else value-pool))
 def stop(m,a,size,unused):
  if a==0x400000:m.emu_stop()
 u.hook_add(UC_HOOK_MEM_WRITE,store);u.hook_add(UC_HOOK_CODE,stop);u.emu_start(names['_omf_initialize_records']if side else row['start'],0,count=10000);assert u.reg_read(UC_X86_REG_ESP)==0x300804
 expected=bytearray(data)
 for i in range(4):expected[i*1004:i*1004+4]=bytes(4)
 assert bytes(u.mem_read(pool,len(data)))==expected and struct.unpack('<I',u.mem_read(current,4))[0]==pool
 assert writes==[(i*1004,4,0)for i in range(4)]+[('current',4,0)]
 return writes
rows=[]
for poison in [0,1,0x5a,0xff]:
 a=run(original,False,poison);b=run(canonical,True,poison);assert a==b;rows.append(dict(poison=poison,writes=a))
Path('cases/omf-record-cache-validation.json').write_text(json.dumps(dict(actual_zero_input_abi=True,rejected=rejected,records=4,record_bytes=1004,typed_header_bytes=20,wire_storage_bytes_per_record=984,original_pool=['0x7a8dc','0x7b88c'],controls=rows,seconds=time.perf_counter()-started,scope='Whole34-byte original/canonical function, exact four tag writes/current-record assignment and every other pool byte unchanged under four poison patterns. Original/canonical globals lie in BSS, zero implicit initial storage; no raw data/section copy.1004-byte slot stride/end from original initializer and cache lookup;20-byte header/context/count/cursor and serialized wire bytes from6c060/6c330 producers. These later functions remain unaccepted; analysis establishes layout only. True zero-argument/hidden-input ABI observed separately; no buffer output or case completion claimed.'),indent=2)+'\n');print('4 whole OMF cache resets,3 ABI rejects, typed BSS/slot identities pass')
