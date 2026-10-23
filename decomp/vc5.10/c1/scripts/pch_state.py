"""Typed PCH file/relocation context, genuine member ABI and bounded reset."""
import json,re,struct,subprocess
from provenance import captured_native_binary
from pass_state import plain_globals_after_invoke_store
MEMBERS={'image_count':0,'root_image':4,'images':8,'stored_file_position':12,'file_name':16,'file_offset':20,'file':24,'mapping_handles':28,'loaded_image_count':52,'scratch_arena':56,'original_pages':60,'mapped_pages':64}
POINTERS=['root_image','images','file_name','file']+['mapping_handles[%d]'%i for i in range(6)]+['scratch_arena','original_pages','mapped_pages']
METHOD='CompilerPchState::Initialize'
LINKAGE='_ZN16CompilerPchState10InitializeEv'
PLAN=dict(entry_rva=0x1279c,pointer_roles='owned snapshot interior sentinels; no handle API or semantic dereference before the named member trap',pointer_first_offset=1024,pointer_stride=8,image_count=37,loaded_image_count=19,stored_file_position=0x11223344,file_offset=0x55667788,normal_case=False)
def zero():return {key:[0]*6 if key=='mapping_handles' else 0 for key in MEMBERS}
def seeded(allocation):
 result=zero();result.update(image_count=37,loaded_image_count=19,stored_file_position=0x11223344,file_offset=0x55667788)
 for i,key in enumerate(POINTERS):
  value=allocation+1024+i*8
  if key.startswith('mapping_handles['):result['mapping_handles'][int(key[-2])]=value
  else:result[key]=value
 return result

def expected_fields(fields):
 model=fields['pch_state']
 assert (model['original_rva'],model['native_offset'],model['modeled_bytes'],model['original_object_extent_unknown'])==(0x58f68,708,68,True)
 assert model['members']==MEMBERS and model['mapping_handle_count']==6
 assert (model['native_method'],model['native_linkage_symbol'],model['member_rva'],model['larger_image_arena_layouts_known'])==(METHOD,LINKAGE,0x11e2a,False)
 row=next(r for r in fields['modeled_bss'] if r['name']=='pch_state')
 assert row['default']==zero() and row['native_field']=='c1_bss.pch_state'
 assert row['members']==[dict(name=name,offset=offset,bytes=24 if name=='mapping_handles' else 4) for name,offset in MEMBERS.items()]
 ranges=[(r['offset'],r['offset']+r['bytes']) for r in row['members']]
 assert ranges[0][0]==0 and ranges[-1][1]==68 and all(a[1]==b[0] for a,b in zip(ranges,ranges[1:]))
 return {'pch_state':(0x58f68,68,708)}
def read(raw):
 result={}
 for name,offset in MEMBERS.items():
  result[name]=list(struct.unpack_from('<6I',raw,0x8f68+offset)) if name=='mapping_handles' else struct.unpack_from('<i' if name in ('image_count','loaded_image_count') else '<I',raw,0x8f68+offset)[0]
 return result

def at(original,sequence,native,wanted):
 raw=(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 assert read(raw)==native['bss']['pch_state']==wanted
 return raw

def validate(original,native,fields,state,sequence,probe,candidate,project):
 joint=any(row['name']==METHOD for row in candidate['functions'])
 expected_fields(fields);assert candidate['restore_probe']['pch_state']==PLAN
 assert read((original/'compiler-bss-000.bin').read_bytes())==zero()
 current=state['compiler_state'];raw=at(original,sequence,current,zero())
 events=(original/'events.txt').read_text();module=int(re.search(r'module name=c1n32_l.dll actual=([0-9a-f]+)',events)[1],16)
 entries=re.findall(r'^trap seq=(\d+) rva=00011e2a esp=([0-9a-f]+) eax=[0-9a-f]+ ecx=([0-9a-f]+) stack=([0-9a-f,]+)$',events,re.M)
 assert len(entries)==1
 seq,esp,receiver,stack=entries[0]
 assert (int(seq)<sequence if joint else int(seq)==sequence) and int(receiver,16)==module+0x58f68 and int(stack.split(',')[0],16)==module+0x12348
 if joint:
  if candidate['expected_frontier']['kind']=='compiler-main':assert set(state['args'])=={'argc','argv'} and candidate['expected_frontier']['rva']==0x127a7
  else:assert state['args']=={} and candidate['expected_frontier']['rva']==0x127a6
 else:assert state['args']==dict(this=current['live_addresses']['bss']+708)
 binary=captured_native_binary(native);symbols=subprocess.check_output(['nm','-S','--defined-only',str(binary)],text=True)
 address,size=(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] '+re.escape(LINKAGE)+'$',symbols,re.M).groups())
 observations=json.loads((native/'pch-entry-observations.json').read_text());assert len(observations)==1
 entry=observations[0]
 assert entry['pc']==address and entry['ecx']==current['live_addresses']['bss']+708 and entry['esp']
 assert len(entry['formal_inventory'])==1 and entry['formal_inventory'][0]==dict(name='this',type='CompilerPchState *')
 caller,caller_size=(int(v,16) for v in re.search(r'^([0-9a-f]+) ([0-9a-f]+) [Tt] initialize_state_1279c$',symbols,re.M).groups())
 assert caller<entry['return']<caller+caller_size
 call=subprocess.check_output(['objdump','-d','--start-address='+str(entry['return']-5),'--stop-address='+str(entry['return']),str(binary)],text=True)
 assert re.search(r'^\s*%x:\s+e8 (?:[0-9a-f]{2} ){3}[0-9a-f]{2}\s+call\s+%x\b'%(entry['return']-5,address),call,re.M)
 code=subprocess.check_output(['objdump','-d','-Mintel','--start-address='+str(address),'--stop-address='+str(address+size),str(binary)],text=True)
 returns=re.findall(r'\bret[ \t]*([^\n]*)',code);assert returns and all(value.strip()=='' for value in returns)
 if joint:validate_member(original,native,fields,state,sequence,probe,candidate,module,int(seq),entry,project)
 if probe=='normal':
  assert not (original/'pch-state-probe.txt').exists() and not re.findall(r'^pch-state-seed ',events,re.M)
  return
 assert (original/'pch-state-probe.txt').read_text()=='58f68 44\n'
 obs=json.loads((native/'restore-observations.json').read_text());assert obs['pre_seed']['bss']['pch_state']==zero()
 allocation=struct.unpack_from('<I',raw,0x7818)[0];native_allocation=current['bss']['saved_data']
 assert re.findall(r'^pch-state-seed .*$',events,re.M)==['pch-state-seed rva=000122a5 object=00058f68 allocation=%08x bytes=68'%allocation]
 for rva,key in [(0x122a5,'before'),(0x303d5,'post_restore'),(0x12365,'before_callback'),(0x12377,'before_current_callback'),(0x12389,'before_contexts_callback'),(0x12646,'before_value_copy'),(0x12663,'after_value_copy'),(0x12664,'before_signature'),(0x1266f,'after_signature'),(0x12676,'before_pdb_context'),(0x126af,'after_pdb_context'),(0x126b6,'before_output_pdb_context'),(0x126ef,'after_output_pdb_context'),(0x1277b,'before_prototype')]:
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%rva,events,re.M);assert len(seqs)==1
  if key=='before':
   old=(original/('compiler-bss-%03d.bin'%int(seqs[0]))).read_bytes()
   assert read(old)==seeded(allocation) and obs[key]['bss']['pch_state']==seeded(native_allocation)
  else:at(original,int(seqs[0]),obs[key],zero())
 for rva,item in obs['buffer_constructors'].items():
  seqs=re.findall(r'^trap seq=(\d+) rva=%08x '%int(rva),events,re.M);assert len(seqs)==1
  at(original,int(seqs[0]),item['before'],zero())


def validate_member(original,native,fields,state,sequence,probe,candidate,module,entry_sequence,entry,project):
 chain=candidate.get('joint_acceptance',candidate.get('accepted_joint_prefix'))
 assert chain['kind']=='ordered-tail-chain' and chain['functions']==['_initialize_state_1279c',METHOD]
 member=next(row for row in candidate['functions'] if row['name']==METHOD)
 wrapper=next(row for row in candidate['functions'] if row['name']=='_initialize_state_1279c')
 assert member['chunks']==[[0x11e2a,0x11e3c]] and wrapper['chunks']==[[0x1279c,0x127a6]]
 if candidate.get('joint_acceptance'):
  assert member['status'].startswith('joint-new-candidate') and wrapper['status'].startswith('joint-new-candidate')
 else:
  assert member['status']==wrapper['status']=='accepted-prefix'
  durable=json.loads((project/'functions.json').read_text())
  assert all(name in durable['accepted'] for name in chain['functions'])
 relocated=(original/'relocated-text.bin').read_bytes()
 assert relocated[0x11e2a-0x1000:0x11e3c-0x1000]==bytes.fromhex('8b d1 57 b9 11 00 00 00 33 c0 8b fa f3 ab 8b c2 5f c3')
 assert relocated[0x1279c-0x1000:0x127a1-0x1000]==b'\xb9'+struct.pack('<I',module+0x58f68)
 assert relocated[0x127a1-0x1000]==0xe9 and 0x127a6+struct.unpack_from('<i',relocated,0x127a2-0x1000)[0]==0x11e2a
 events=(original/'events.txt').read_text()
 wrappers=re.findall(r'^trap seq=(\d+) rva=0001279c ',events,re.M);assert len(wrappers)==1 and int(wrappers[0])<entry_sequence
 returned=re.findall(r'^pch-member-return seq=(\d+) pc=([0-9a-f]+) eax=([0-9a-f]+) esp=([0-9a-f]+) entry-esp=([0-9a-f]+) edi=([0-9a-f]+) entry-edi=([0-9a-f]+)$',events,re.M);assert len(returned)==1
 rseq,pc,eax,esp,entry_esp,edi,entry_edi=[int(x,10 if i==0 else 16) for i,x in enumerate(returned[0])]
 assert entry_sequence<rseq<sequence and pc==module+0x12348 and eax==module+0x58f68 and esp==entry_esp+4 and edi==entry_edi
 observed_entry=re.findall(r'^pch-member-entry seq=(\d+) edi=([0-9a-f]+) inherited-return=([0-9a-f]+)$',events,re.M)
 assert len(observed_entry)==1 and [int(x,10 if i==0 else 16) for i,x in enumerate(observed_entry[0])]==[entry_sequence,edi,pc]
 original_entry=re.findall(r'^trap seq=%d rva=00011e2a esp=([0-9a-f]+) '%entry_sequence,events,re.M);assert len(original_entry)==1 and int(original_entry[0],16)==entry_esp
 original_before=(original/('compiler-bss-%03d.bin'%entry_sequence)).read_bytes()
 original_after=(original/('compiler-bss-%03d.bin'%rseq)).read_bytes()
 wanted=bytearray(original_before);wanted[0x8f68:0x8fac]=bytes(68)
 assert bytes(wanted)==original_after and original_after==(original/('compiler-bss-%03d.bin'%sequence)).read_bytes()
 for region in ('compiler-data','compiler-snapshot','state'):
  before_region=(original/('%s-%03d.bin'%(region,entry_sequence))).read_bytes()
  returned_region=(original/('%s-%03d.bin'%(region,rseq))).read_bytes()
  assert before_region==returned_region
  expected_region=plain_globals_after_invoke_store(original,returned_region,candidate,module) if region=='state' else returned_region
  assert expected_region==(original/('%s-%03d.bin'%(region,sequence))).read_bytes()
 native_returns=json.loads((native/'pch-return-observations.json').read_text());assert len(native_returns)==1
 returned_native=native_returns[0]
 assert returned_native['pc']==entry['return'] and returned_native['eax']==entry['ecx'] and returned_native['esp']==entry['esp']+4 and returned_native['edi']==entry['edi']
 assert returned_native['values']==[0]*17
 native_expected=list(entry['bss_bytes']);assert len(native_expected)==776
 native_expected[708:776]=[0]*68
 assert returned_native['bss_bytes']==native_expected
 if probe=='normal':
  assert read(original_before)==zero() and entry['values']==[0]*17
  assert not (original/'pch-member-probe.txt').exists() and not re.findall(r'^pch-member-seed ',events,re.M)
 else:
  assert candidate['restore_probe']['pch_member']==dict(entry_rva=0x11e2a,object_rva=0x58f68,bytes=68,normal_case=False,pointer_roles='owned snapshot interior sentinels; cleared by genuine member before any semantic consumer')
  assert (original/'pch-member-probe.txt').read_text()=='11e2a 58f68 44\n'
  allocation=struct.unpack_from('<I',original_before,0x7818)[0]
  assert read(original_before)==seeded(allocation)
  native_allocation=state['compiler_state']['bss']['saved_data']
  native_raw=bytearray(0x8fac);native_raw[0x8f68:0x8fac]=struct.pack('<17I',*entry['values'])
  assert read(native_raw)==seeded(native_allocation)
  assert re.findall(r'^pch-member-seed .*$',events,re.M)==['pch-member-seed rva=00011e2a object=00058f68 allocation=%08x bytes=68'%allocation]
