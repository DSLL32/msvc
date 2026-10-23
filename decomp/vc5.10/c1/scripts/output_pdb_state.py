"""Second encountered persistent PDB context; provider objects remain opaque."""
import json,re,struct,subprocess
import pdb_state
from provenance import captured_native_binary

def read(raw):return {key:struct.unpack_from('<I',raw,0x70+offset)[0] for key,offset in pdb_state.MEMBERS.items()}
def seeded(allocation):return pdb_state.seed(allocation+64)
def at(original,sequence,native,allocation,native_allocation,seed):
 raw=(original/('state-%03d.bin'%sequence)).read_bytes()
 assert read(raw)==(seeded(allocation) if seed else pdb_state.zero())
 assert native['output_pdb_context']==(seeded(native_allocation) if seed else pdb_state.zero())
 return raw

def validate(original,native,fields,state,sequence,probe,candidate,project):
 model=fields['output_pdb_context']
 assert model==dict(fields['pdb_context'],original_base_rva=0x5c070)
 rows=fields['modeled_persistent'][6:];assert len(rows)==6
 for row,(name,offset) in zip(rows,pdb_state.MEMBERS.items()):
  assert (row['name'],row['original_rva'],row['bytes'],row['native_offset'],row['native_field'],row['default'])==('output.'+name,0x5c070+offset,4,pdb_state.NATIVE[name],'c1_output_pdb_context.'+name,0)
 assert read((original/'state-000.bin').read_bytes())==pdb_state.zero()
 current=state['compiler_state'];layout=current['output_pdb_context_layout']
 symbols=subprocess.check_output(['nm','-S','--defined-only',str(captured_native_binary(native))],text=True)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [A-Za-z] c1_output_pdb_context$',symbols,re.M).groups()
 assert layout==dict(address=int(address,16),size=int(size,16),members=pdb_state.NATIVE) and int(size,16)==24
 intervals=[(layout['address'],layout['address']+24),(current['pdb_context_layout']['address'],current['pdb_context_layout']['address']+24)]+[(current['live_addresses'][key],current['live_addresses'][key]+current['native_layout'][kind]) for key,kind in [('initialized','initialized_size'),('bss','bss_size')]]
 intervals.sort();assert all(left[1]<=right[0] for left,right in zip(intervals,intervals[1:]))
 receipt=json.loads((native/'receipt.json').read_text());reuse=receipt['pdb_reuse'] if probe=='restore' else 0
 events=(original/'events.txt').read_text();module=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',events)[1],16)
 allocation=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%sequence)).read_bytes(),0x7818)[0];native_allocation=current['bss']['saved_data']
 at(original,sequence,current,allocation,native_allocation,seed=bool(reuse))
 if probe=='normal':return
 obs=json.loads((native/'restore-observations.json').read_text());assert obs['pre_seed']['output_pdb_context']==pdb_state.zero()
 for rva,key in [(0x122a5,'before'),(0x303d5,'post_restore'),(0x12365,'before_callback'),(0x12377,'before_current_callback'),(0x12389,'before_contexts_callback'),(0x12646,'before_value_copy'),(0x12663,'after_value_copy'),(0x12664,'before_signature'),(0x1266f,'after_signature'),(0x12676,'before_pdb_context'),(0x126af,'after_pdb_context'),(0x126b6,'before_output_pdb_context')]:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1 and int(seqs[0])<sequence
  at(original,int(seqs[0]),obs[key],allocation,native_allocation,seed=True)
 for rva,item in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),item['before'],allocation,native_allocation,seed=True)
 assert re.search(r'^output-pdb-context-seed rva=000122a5 allocation=%08x read_mode=37$'%allocation,events,re.M)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_pdb_context_126b6$',symbols,re.M).groups()
 assert int(address,16)<=obs['output_pdb_context_pc']<int(address,16)+int(size,16)
 if 'after_output_pdb_context' in obs:
  current=obs['after_output_pdb_context']
  sequence=int(re.findall(r'^trap seq=(\d+) rva=000126ef ',events,re.M)[0])
  after_address,after_size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_126ef$',symbols,re.M).groups()
  assert int(after_address,16)<=obs['after_output_pdb_context_pc']<int(after_address,16)+int(after_size,16)
  at(original,sequence,current,allocation,native_allocation,seed=bool(reuse))
 before=obs['before_output_pdb_context'];assert before['reuse_state']==current['reuse_state']==reuse
 seq=int(re.findall(r'^trap seq=(\d+) rva=000126b6 ',events,re.M)[0])
 old_raw=(original/('state-%03d.bin'%seq)).read_bytes();new_raw=(original/('state-%03d.bin'%sequence)).read_bytes();expected=bytearray(old_raw)
 if not reuse:
  for offset in pdb_state.MEMBERS.values():struct.pack_into('<I',expected,0x70+offset,0)
 assert bytes(expected)==new_raw
 native_symbols={name:int(address,16) for address,size,name in re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [TtWw] (\S+)$',symbols,re.M)}
 count=next(i+1 for i,row in enumerate(candidate['registered_cleanup']) if row['native_symbol']=='destroy_pdb_context_238db');assert before['bss']['cleanup_count']==count-1
 after_bss=dict(before['bss']);after_bss['cleanup_count']=count;after_bss['cleanup_callbacks']=list(after_bss['cleanup_callbacks']);after_bss['cleanup_callbacks'][count-1]=native_symbols['destroy_pdb_context_238db'];assert after_bss==current['bss']
 for key in before:
  if key not in ('output_pdb_context','bss'):assert before[key]==current[key]
 raw=bytearray((original/('compiler-bss-%03d.bin'%seq)).read_bytes());struct.pack_into('<I',raw,0x781c,count);struct.pack_into('<I',raw,0x7820+(count-1)*4,module+0x238db)
 assert bytes(raw)==(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
