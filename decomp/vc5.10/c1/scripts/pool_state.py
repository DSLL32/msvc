"""Encountered pool fields and genuine i386 thiscall boundary, not an arena layout."""
import json,re,struct,subprocess
from provenance import captured_native_binary

MEMBERS={'cursor':0,'remaining':4,'minimum_pages':8,'arena':12}
METHOD='CompilerPool::Initialize'
LINKAGE='_ZN12CompilerPool10InitializeEj'
PLAN=dict(cursor_offset=128,arena_offset=512,instance_stride=16,remaining_base=91,
          minimum_pages_base=7,pointer_roles='owned snapshot interiors; never dereferenced before named member trap',normal_case=False)

def expected_fields(fields):
 models=fields['pools'];assert len(models)==1
 result={}
 for index,model in enumerate(models):
  assert (model['index'],model['original_object_rva'],model['native_offset'],model['native_size'])==(index,0x55078,544,16)
  assert model['original_object_extent_unknown'] is True
  assert model['members']==MEMBERS and model['member_rva']==0x126fc
  assert model['native_method']==METHOD and model['native_linkage_symbol']==LINKAGE and model['constructor_argument']==1
  for name,offset in MEMBERS.items():
   key='pools[%d].%s'%(index,name)
   row=next(r for r in fields['modeled_bss'] if r['name']==key)
   assert row['default']==0 and row['native_field']=='c1_bss.'+key
   result[key]=(model['original_object_rva']+offset,4,model['native_offset']+offset)
 return result

def read(raw,model):
 return {name:struct.unpack_from('<I',raw,model['original_object_rva']-0x50000+offset)[0] for name,offset in MEMBERS.items()}

def zero():return dict.fromkeys(MEMBERS,0)
def seeded(allocation,index):
 return dict(cursor=allocation+128+index*16,remaining=91+index,minimum_pages=7+index,arena=allocation+512+index*16)

def at(original,sequence,native,models,allocation=0,native_allocation=0,seed=False):
 raw=(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 assert len(native['bss']['pools'])==len(models)
 for index,model in enumerate(models):
  assert read(raw,model)==(seeded(allocation,index) if seed else zero())
  assert native['bss']['pools'][index]==(seeded(native_allocation,index) if seed else zero())

def validate(original,native,fields,state,sequence,probe,candidate,project):
 expected_fields(fields);models=fields['pools']
 assert candidate['restore_probe']['pools']==PLAN
 for model in models:assert read((original/'compiler-bss-000.bin').read_bytes(),model)==zero()
 current=state['compiler_state']
 assert len(current['bss']['pools'])==len(models)
 member=any(row['name']==METHOD for row in candidate['functions'])
 if member:
  raw=(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
  result=dict(cursor=0,remaining=0,minimum_pages=1,arena=0)
  assert read(raw,models[0])==current['bss']['pools'][0]==result
 else:at(original,sequence,current,models)
 events=(original/'events.txt').read_text()
 module=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',events)[1],16)
 entries=re.findall(r'^trap seq=(\d+) rva=000126fc .*ecx=([0-9a-f]+).*stack=([0-9a-f,]+)$',events,re.M)
 assert len(entries)==1
 seq,receiver,stack=entries[0];words=[int(word,16) for word in stack.split(',')]
 assert (int(seq)<sequence if member else int(seq)==sequence) and int(receiver,16)==module+models[0]['original_object_rva']
 assert words[0]==module+0x126fb and words[1]==1
 if not member:assert state['args']==dict(this=current['live_addresses']['bss']+544,minimum_pages=1)
 binary=captured_native_binary(native)
 symbols=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
 address,size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+re.escape(LINKAGE)+'$',symbols,re.M).groups()
 address,size=int(address,16),int(size,16)
 caller,caller_size=re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_126ef$',symbols,re.M).groups()
 caller,caller_size=int(caller,16),int(caller_size,16)
 observations=json.loads((native/'pool-entry-observations.json').read_text())
 assert len(observations)==1
 entry=observations[0]
 assert entry['pc']==address and entry['ecx']==current['live_addresses']['bss']+544 and entry['argument']==1 and entry['esp']
 assert caller<entry['return']<caller+caller_size
 call=subprocess.check_output(['objdump','-d','--start-address='+str(entry['return']-5),'--stop-address='+str(entry['return']),str(binary)],text=True)
 assert re.search(r'^\s*%x:\s+e8 (?:[0-9a-f]{2} ){3}[0-9a-f]{2}\s+call\s+%x\b'%(entry['return']-5,address),call,re.M),'raw return must follow this member call'
 code=subprocess.check_output(['objdump','-d','-Mintel','--start-address='+str(address),'--stop-address='+str(address+size),str(binary)],text=True)
 returns=re.findall(r'\bret\s*([^\n]*)',code)
 assert returns and all(value.strip()=='0x4' for value in returns),'native member must preserve RET4'
 if member:
  allocation=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%int(seq))).read_bytes(),0x7818)[0]
  native_allocation=current['bss']['saved_data']
  assert read((original/('compiler-bss-%03d.bin'%int(seq))).read_bytes(),models[0])==(seeded(allocation,0) if probe=='restore' else zero())
  expected_seed=seeded(native_allocation,0) if probe=='restore' else zero()
  assert entry['values']==list(expected_seed.values())
  returned=re.findall(r'^pool-return seq=(\d+) pc=([0-9a-f]+) eax=([0-9a-f]+) esp=([0-9a-f]+) entry-esp=([0-9a-f]+)$',events,re.M)
  assert len(returned)==1
  postseq,pc,eax,esp,entry_esp=returned[0]
  assert int(seq)<int(postseq)<sequence and int(pc,16)==words[0] and int(eax,16)==module+models[0]['original_object_rva']
  original_esp=int(re.search(r'^trap seq='+seq+r' rva=000126fc esp=([0-9a-f]+)',events,re.M)[1],16)
  assert int(entry_esp,16)==original_esp and int(esp,16)==original_esp+8
  returned_native=json.loads((native/'pool-return-observations.json').read_text());assert len(returned_native)==1
  after=returned_native[0]
  assert after==dict(pc=entry['return'],eax=entry['ecx'],esp=entry['esp']+8,values=list(result.values()))
  old=bytearray((original/('compiler-bss-%03d.bin'%int(seq))).read_bytes())
  for name,offset in MEMBERS.items():struct.pack_into('<I',old,0x5078+offset,result[name])
  assert bytes(old)==(original/('compiler-bss-%03d.bin'%int(postseq))).read_bytes()==(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
  for region in ('compiler-data','compiler-snapshot','state'):
   assert (original/('%s-%03d.bin'%(region,int(seq)))).read_bytes()==(original/('%s-%03d.bin'%(region,int(postseq)))).read_bytes()
  for kind in ('last-error','x87-control'):
   before=re.search(r'^trap seq='+seq+r' .*\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M)
   after_match=re.search(r'^pool-return .*\nlast-error=(\d+)\nx87-control=([0-9a-f]+)$',events,re.M)
   assert before and after_match and before.groups()==after_match.groups()
  if probe=='restore':
   assert candidate['restore_probe']['pool_member']==dict(entry_rva=0x126fc,object_index=0,pointer_roles='owned snapshot interior sentinels, never dereferenced',minimum_pages=1,normal_case=False)
   assert (original/'pool-member-probe.txt').read_text()=='126fc 0\n'
   assert re.search(r'^pool-member-seed rva=000126fc object=00055078 allocation=%08x$'%allocation,events,re.M)
   assert re.findall(r'^pool-member-field-seed .*$',events,re.M)==['pool-member-field-seed index=0 object=00055078 allocation=%08x'%allocation]
 if probe=='normal':
  assert not (original/'pools-probe.txt').exists() and not re.findall(r'^pool-seed ',events,re.M)
  return
 assert (original/'pools-probe.txt').read_text()==''.join('%x\n'%model['original_object_rva'] for model in models)
 obs=json.loads((native/'restore-observations.json').read_text())
 assert obs['pre_seed']['bss']['pools']==[zero() for model in models]
 allocation=struct.unpack_from('<I',(original/('compiler-bss-%03d.bin'%sequence)).read_bytes(),0x7818)[0]
 native_allocation=current['bss']['saved_data']
 for index,model in enumerate(models):
  assert re.findall(r'^pool-seed index=%d .*'%index,events,re.M)==['pool-seed index=%d object=%08x allocation=%08x'%(index,model['original_object_rva'],allocation)]
 for rva,key in [(0x122a5,'before'),(0x303d5,'post_restore'),(0x12365,'before_callback'),(0x12377,'before_current_callback'),(0x12389,'before_contexts_callback'),(0x12646,'before_value_copy'),(0x12663,'after_value_copy'),(0x12664,'before_signature'),(0x1266f,'after_signature'),(0x12676,'before_pdb_context'),(0x126af,'after_pdb_context'),(0x126b6,'before_output_pdb_context')]:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),obs[key],models,allocation,native_allocation,seed=key=='before')
 for rva,item in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),item['before'],models)
