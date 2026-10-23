"""Signed numeric prefix of a wider value; its 16-byte tail stays UNMODELED."""
import json,re,struct,subprocess
from provenance import captured_native_binary
FIELDS={'literal.integer':(0x518a0,8,520),'saved_literal.integer':(0x55108,8,528),'saved_literal_available':(0x55100,1,536)}
PLAN=dict(before_clear_source=-0x223344556677889,before_clear_cached=0x3344556677881122,positive_source=0x1122334455667788,negative_source=-0x112233445566778,cached_before_copy=0x5566778811223344,flag_before=165,flag_after=1,entry_rva=0x12646,normal_case=False)
def expected_fields(fields,frontier=None):
 m=fields['value_copy']
 wanted=dict(original_source_rva=0x518a0,original_destination_rva=0x55108,original_record_bytes=24,modeled_integer_bytes=8,unmodeled_tail_bytes=16,flag_rva=0x55100,native_literal_offset=520,native_saved_offset=528,native_flag_offset=536,native_value_size=8,native_bss_size=1092 if fields.get('command_line')else 996)
 assert {k:m[k] for k in wanted}==wanted
 if frontier is not None:assert m['hard_frontier_rva']==frontier
 rows={r['name']:r for r in fields['modeled_bss']}
 for name in FIELDS:
  assert rows[name]['default']==0 and rows[name]['native_field']=='c1_bss.'+name
 return FIELDS

def read(raw):return struct.unpack_from('<q',raw,0x18a0)[0],struct.unpack_from('<q',raw,0x5108)[0],raw[0x5100]
def native_values(s):return tuple(s['bss'][key] for key in ('literal','saved_literal','saved_literal_available'))
def pair_at(original,seq,n,wanted):
 raw=(original/('compiler-bss-%03d.bin'%seq)).read_bytes()
 assert read(raw)==native_values(n)==wanted
 return raw

def validate(original,native,fields,state,sequence,probe,candidate,project):
 expected_fields(fields,candidate.get('projection_actual_frontier',candidate['expected_frontier']['rva']))
 assert read((original/'compiler-bss-000.bin').read_bytes())==(0,0,0)
 sign=json.loads((native/'receipt.json').read_text()).get('value_copy_sign')
 plan=candidate['restore_probe']['value_copy'];assert plan==PLAN
 value=plan['negative_source' if sign=='negative' else 'positive_source'] if probe=='restore' else 0
 raw=pair_at(original,sequence,state['compiler_state'],(value,value,1))
 events=(original/'events.txt').read_text()
 entries=re.findall(r'^trap seq=(\d+) rva=00012646 ',events,re.M)
 assert len(entries)==1 and int(entries[0])<sequence
 before=(original/('compiler-bss-%03d.bin'%int(entries[0]))).read_bytes()
 post_entries=re.findall(r'^trap seq=(\d+) rva=00012663 ',events,re.M)
 assert len(post_entries)==1 and int(entries[0])<int(post_entries[0])<=sequence
 copied=(original/('compiler-bss-%03d.bin'%int(post_entries[0]))).read_bytes()
 # Entire-record comparison is ORIGINAL-to-ORIGINAL only; native has 8 known bytes.
 assert copied[0x18a0:0x18b8]==before[0x18a0:0x18b8]
 assert copied[0x5108:0x5120]==before[0x18a0:0x18b8]
 if probe=='normal':
  assert sign is None and read(before)==(0,0,0)
  assert not re.findall(r'^value-seed ',events,re.M)
  assert not (original/'value-probe.txt').exists()
  return
 assert sign in ('positive','negative')
 values=[plan['before_clear_source'],plan['before_clear_cached'],value,plan['cached_before_copy']]
 words=[w for v in values for w in (v&0xffffffff,(v>>32)&0xffffffff)]+[165]
 assert (original/'value-probe.txt').read_text()==' '.join('%x'%w for w in words)+'\n'
 obs=json.loads((native/'restore-observations.json').read_text())
 assert native_values(obs['pre_seed'])==(0,0,0)
 for rva,key,wanted in [(0x122a5,'before',(values[0],values[1],165)),(0x303d5,'post_restore',(0,0,0)),(0x12365,'before_callback',(0,0,0)),(0x12377,'before_current_callback',(0,0,0)),(0x12389,'before_contexts_callback',(0,0,0)),(0x12646,'before_value_copy',(value,values[3],165))]:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M)
  assert len(seqs)==1 and int(seqs[0])<sequence
  pair_at(original,int(seqs[0]),obs[key],wanted)
 for rva,item in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  pair_at(original,int(seqs[0]),item['before'],(0,0,0))
 for rva,source,cached in [(0x122a5,values[0],values[1]),(0x12646,value,values[3])]:
  assert re.search(r'^value-seed rva=%08x source=%016x cached=%016x flag=165$'%(rva,source&0xffffffffffffffff,cached&0xffffffffffffffff),events,re.M)
 symbols=subprocess.check_output(['nm','-S','--defined-only',str(captured_native_binary(native))],text=True)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_12646$',symbols,re.M).groups()
 assert int(address,16)<=obs['value_copy_pc']<int(address,16)+int(size,16)
 old=obs['before_value_copy'];final=obs['after_value_copy']
 post_entries=re.findall(r'^trap seq=(\d+) rva=00012663 ',events,re.M)
 assert len(post_entries)==1
 pair_at(original,int(post_entries[0]),final,(value,value,1))
 native_symbols=subprocess.check_output(['nm','-S','--defined-only',str(captured_native_binary(native))],text=True)
 after_address,after_size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_12663$',native_symbols,re.M).groups()
 assert int(after_address,16)<=obs['after_value_copy_pc']<int(after_address,16)+int(after_size,16)
 for key in old:
  if key=='bss':
   excluded=('saved_literal','saved_literal_available')
   assert {k:v for k,v in old[key].items() if k not in excluded}=={k:v for k,v in final[key].items() if k not in excluded}
  else:assert old[key]==final[key]
 altered=bytearray(before);altered[0x5100]=1;altered[0x5108:0x5120]=before[0x18a0:0x18b8]
 assert bytes(altered)==copied
