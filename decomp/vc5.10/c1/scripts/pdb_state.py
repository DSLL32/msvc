"""Typed persistent PDB session fields, outside datafe snapshot/BSS reset."""
import json,re,struct,subprocess
from provenance import captured_native_binary
MEMBERS={'database':0,'names':4,'types':8,'minimal_rebuild':12,'rebuild_state':20,'read_mode':24}
NATIVE={key:i*4 for i,key in enumerate(MEMBERS)}
PLAN={'pointer_role':'owned snapshot interior sentinels, never dereferenced','pointer_offsets':[0,8,16,24,32],'read_mode':37,'callback_reuse_modes':[0,1],'entry_rva':0x12676,'normal_case':False}
def raw_values(raw):return {key:struct.unpack_from('<I',raw,0x48+offset)[0] for key,offset in MEMBERS.items()}
def zero():return {key:0 for key in MEMBERS}
def seed(allocation):return {key:(37 if key=='read_mode' else allocation+list(MEMBERS).index(key)*8) for key in MEMBERS}
def at(original,sequence,native,allocation,native_allocation,seeded):
 raw=(original/('state-%03d.bin'%sequence)).read_bytes()
 assert raw_values(raw)==(seed(allocation) if seeded else zero())
 assert native['pdb_context']==(seed(native_allocation) if seeded else zero())
 return raw

def validate(original,native,fields,state,sequence,probe,candidate,project):
 model=fields['pdb_context']
 assert model=={'original_base_rva':0x5c048,'original_unmodeled_offsets':[[16,20]],'native_size':24,'native_members':NATIVE,'lifetime':'plain .data: outside both datafe snapshot and bssfe clear','provider_layout_claimed':False}
 rows=fields['modeled_persistent'];assert len(rows)==6
 for row,(name,original_offset) in zip(rows,MEMBERS.items()):
  assert (row['name'],row['original_rva'],row['bytes'],row['native_offset'],row['native_field'],row['default'])==(name,0x5c048+original_offset,4,NATIVE[name],'c1_pdb_context.'+name,0)
  assert not 0x5e000<=row['original_rva']<0x5f994 and not 0x50000<=row['original_rva']<0x5a06c
 assert raw_values((original/'state-000.bin').read_bytes())==zero()
 current=state['compiler_state'];layout=current['pdb_context_layout']
 symbol_text=subprocess.check_output(['nm','-S','--defined-only',str(captured_native_binary(native))],text=True)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [A-Za-z] c1_pdb_context$',symbol_text,re.M).groups()
 assert layout=={'address':int(address,16),'size':int(size,16),'members':NATIVE} and int(size,16)==24
 intervals=[(layout['address'],layout['address']+24)]+[(current['live_addresses'][key],current['live_addresses'][key]+current['native_layout'][kind]) for key,kind in [('initialized','initialized_size'),('bss','bss_size')]]
 intervals.sort();assert all(left[1]<=right[0] for left,right in zip(intervals,intervals[1:]))
 assert candidate['restore_probe']['pdb_context']==PLAN
 receipt=json.loads((native/'receipt.json').read_text());reuse=receipt['pdb_reuse'] if probe=='restore' else 0
 assert reuse in (0,1) and current['reuse_state']==reuse
 events=(original/'events.txt').read_text();module=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',events)[1],16)
 allocation=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%sequence)).read_bytes(),0x7818)[0]
 native_allocation=current['bss']['saved_data']
 at(original,sequence,current,allocation,native_allocation,seeded=bool(reuse))
 if probe=='normal':
  assert receipt['pdb_reuse'] is None
  assert not (original/'pdb-context-probe.txt').exists()
  assert not re.findall(r'^pdb-(context|helper)-seed ',events,re.M)
  return
 assert (original/'pdb-context-probe.txt').read_text()=='%x\n'%reuse
 obs=json.loads((native/'restore-observations.json').read_text());assert obs['pre_seed']['pdb_context']==zero()
 checkpoints=[(0x122a5,'before'),(0x303d5,'post_restore'),(0x12365,'before_callback'),(0x12377,'before_current_callback'),(0x12389,'before_contexts_callback'),(0x12646,'before_value_copy'),(0x12663,'after_value_copy'),(0x12664,'before_signature'),(0x1266f,'after_signature'),(0x12676,'before_pdb_context')]
 for rva,key in checkpoints:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1 and int(seqs[0])<sequence
  at(original,int(seqs[0]),obs[key],allocation,native_allocation,seeded=True)
 for rva,item in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),item['before'],allocation,native_allocation,seeded=True)
 assert re.search(r'^pdb-context-seed rva=000122a5 allocation=%08x read_mode=37$'%allocation,events,re.M)
 assert re.search(r'^pdb-helper-seed rva=00012676 reuse=%d$'%reuse,events,re.M)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_pdb_context_12676$',symbol_text,re.M).groups()
 assert int(address,16)<=obs['pdb_context_pc']<int(address,16)+int(size,16)
 before=obs['before_pdb_context'];assert before['reuse_state']==reuse
 # The helper changes only six mapped session fields. The wrapper then appends
 # exactly its known cleanup callback; no provider object is touched.
 seq=int(re.findall(r'^trap seq=(\d+) rva=00012676 ',events,re.M)[0])
 old_raw=(original/('state-%03d.bin'%seq)).read_bytes();new_raw=(original/('state-%03d.bin'%sequence)).read_bytes()
 expected=bytearray(old_raw)
 if not reuse:
  for offset in MEMBERS.values():struct.pack_into('<I',expected,0x48+offset,0)
 assert bytes(expected)==new_raw
 symbols={name:int(address,16) for address,size,name in re.findall(r'^([0-9a-f]+) ([0-9a-f]+) [TtWw] (\S+)$',symbol_text,re.M)}
 count=len(candidate['registered_cleanup'])
 assert before['bss']['cleanup_count']==count-1
 expected_bss=dict(before['bss']);expected_bss['cleanup_count']=count
 expected_bss['cleanup_callbacks']=list(expected_bss['cleanup_callbacks']);expected_bss['cleanup_callbacks'][count-1]=symbols['destroy_pdb_context_238dc']
 assert expected_bss==current['bss']
 for key in before:
  if key not in ('pdb_context','bss'):assert before[key]==current[key]
 old_bss=bytearray((original/('compiler-bss-%03d.bin'%seq)).read_bytes())
 struct.pack_into('<I',old_bss,0x781c,count);struct.pack_into('<I',old_bss,0x7820+(count-1)*4,module+0x238dc)
 assert bytes(old_bss)==(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
