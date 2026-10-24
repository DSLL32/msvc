"""Owned aligned arena allocation, real return and published thiscall receiver."""
import copy,json,re,struct,subprocess
from integer_state import load,locale_raw
from provenance import captured_native_binary,sha
from vector_state import snapshot
from packing_state import validate as validate_prefix
from allocation_state import symbols,call
from scaler_state import semantic
from signature_call_state import fp
from frontier_contract import validate_original_frontier,validate_native_stop
BASE=0x10600000

def registers(original,phase):return list(struct.unpack('<16I',(original/('packing-allocation-'+phase+'-registers.bin')).read_bytes()))
def arena(original,phase):return list(struct.unpack('<8I',(original/('packing-allocation-'+phase+'-arena.bin')).read_bytes()))

def validate(original,native,candidate,report):
 rows=load(native,'packing-allocation-entries');assert len(rows)==1
 stage=snapshot(original,native,load(native,'state'),rows[0]);prefix=validate_prefix(original,native,candidate,report,stage_state=stage)
 result=validate_endpoint(original,native,candidate);result['prefix']=prefix;return result

def validate_endpoint(original,native,candidate):
 assert candidate['packing_allocator_observers']==dict(entry_rva=0x134c9,return_rva=0x13494,frontier_rva=0xd458,request_bytes=8,arena_kind=0)
 assert candidate['expected_frontier']==dict(kind='packing-allocation-complete',rva=0xd458,native_symbol='CompilerPacking::Push',native_args=['this'],calling_convention='thiscall',caller=dict(native_symbol='initialize_packing',return_rva=0x134a3))and candidate['allowed_probes']==['normal']
 f=[r for r in candidate['functions']if r['name']=='_allocate_packing_memory'];assert len(f)==1 and f[0]['chunks']==[[0x134c9,0x134f4],[0x30b02,0x30b11]]and f[0]['role']=='application'
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,'normal');validate_native_stop(native,candidate)
 binary=captured_native_binary(native);names=symbols(binary);a,z,m=[registers(original,p)for p in ('entry','return','member')];before,after,member=[arena(original,p)for p in ('entry','return','member')]
 assert a[1]==BASE+0x134c9 and a[10:12]==[BASE+0x13494,8]and a[12:]==[before[0],BASE,a[2],8]
 assert before[2]<=before[0]<before[0]+8<=before[1]and before[3]==65536 and before[1]==before[2]+before[3]
 assert z[1]==BASE+0x13494 and z[2]==a[2]+4 and z[3]==a[12]and z[6:10]==a[6:10]and z[10:12]==[8,BASE+0x12d55]and z[12:]==a[12:]
 expected=before[:];expected[0]+=8;assert after==member==expected
 assert m[1]==BASE+0xd458 and m[2]==a[2]+4 and m[4]==a[12]and m[10]==BASE+0x134a3 and m[12:]==a[12:]
 assert z[10]==8 # actual caller-owned incoming byte count, unchanged
 raw={k:bytearray(v)for k,v in locale_raw(original,'packing-allocation-entry').items()}
 assert struct.unpack_from('<I',raw['BSS'],0x8fb0)[0]==a[12]and struct.unpack_from('<I',raw['data'],0x410)[0]==0
 struct.pack_into('<I',raw['BSS'],0x8fb0,a[12]+8)
 for region in raw:assert locale_raw(original,'packing-allocation-return')[region]==bytes(raw[region])
 struct.pack_into('<I',raw['data'],0x410,a[12])
 for region in raw:assert locale_raw(original,'packing-allocation-member')[region]==bytes(raw[region])
 for stem in ('state',):
  assert(original/('%s-%03d.bin'%(stem,a[0]))).read_bytes()==(original/('%s-%03d.bin'%(stem,z[0]))).read_bytes()==(original/('%s-%03d.bin'%(stem,m[0]))).read_bytes()
 old_memory=(original/'packing-allocation-entry-memory.bin').read_bytes();assert len(old_memory)==8 and old_memory==bytes(8)
 for phase in ('return','member'):assert(original/('packing-allocation-'+phase+'-memory.bin')).read_bytes()==old_memory
 es,rs,ms=[load(native,name)for name in ('packing-allocation-entries','packing-allocation-returns','packing-member-entries')];assert len(es)==len(rs)==len(ms)==1;e,r,t=es[0],rs[0],ms[0];state=load(native,'state')
 assert e['pc']==names['allocate_packing_memory'][0]and e['formal_inventory']==[dict(name='bytes',type='unsigned int')]and e['args']=={'bytes':8}and e['words']==[e['caller']['pc'],8]
 assert e['caller']['symbol']=='initialize_packing'and e['caller']['args']=={};call(binary,e['words'][0],e['pc'])
 memory=e['memory_address'];old=copy.deepcopy(e['compiler_state']);initial_arena=old['bss']['arenas'][0]
 assert memory==initial_arena['cursor']and initial_arena['base']<=memory<memory+8<=initial_arena['committed_limit']and initial_arena['committed_bytes']==65536
 assert memory-initial_arena['base']==a[12]-before[2]
 updated=copy.deepcopy(old);updated['bss']['arenas'][0]['cursor']+=8
 assert r['pc']==e['words'][0]and r['esp']==e['esp']+4 and r['eax']==memory and r['preserved']==e['preserved']and r['compiler_state']==updated
 updated['option_state']['initialized']['packing_stack']=memory
 assert t['pc']==names['_ZN15CompilerPacking4PushEv'][0]and t['ecx']==memory and t['args']=={'this':memory}and t['words']==[t['caller']['pc']]
 assert len(t['formal_inventory'])==1 and t['formal_inventory'][0]['name']=='this'and re.fullmatch(r'(?:struct )?CompilerPacking \*(?: const)?',t['formal_inventory'][0]['type'])
 assert t['object_layout']==dict(bytes=8,active=0,free_frames=4)and t['compiler_state']==updated
 assert t['caller']['symbol']=='initialize_packing'and t['caller']['args']=={};call(binary,t['words'][0],t['pc'])
 assert state['symbol']=='CompilerPacking::Push'and state['args']==t['args']and state['formal_inventory']==t['formal_inventory']and state['compiler_state']==updated
 assert names['_ZN15CompilerPacking4PushEv'][0]<state['pc']<=sum(names['_ZN15CompilerPacking4PushEv'])
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert 'int3'in opcode
 assert state['caller']['symbol']=='initialize_packing'and state['caller']['args']=={}and state['caller']['pc']==t['words'][0]
 for row in (e,r,t):assert row['memory']==list(old_memory)and row['invocation']==e['invocation']and row['crt_state']==e['crt_state']and row['errors']==[0,0,0]
 arch=semantic(e['fp']);locale=e['locale']
 for phase,row in zip(('entry','return','member'),(e,r,t)):
  assert semantic(row['fp'])==arch==fp((original/('locale-packing-allocation-'+phase+'-fp.bin')).read_bytes())
  for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv'):
   assert(original/('locale-packing-allocation-'+phase+'-'+suffix+'.bin')).read_bytes()==(original/('locale-packing-allocation-entry-'+suffix+'.bin')).read_bytes()
  assert{k:v for k,v in row['locale'].items()if k not in ('phase','fp','compiler_state')}=={k:v for k,v in locale.items()if k not in ('phase','fp','compiler_state')}
  assert row['locale']['compiler_state']==row['compiler_state']
  assert semantic(row['locale']['fp'])==arch
 assert state['invocation']==e['invocation']and semantic(state['command_line_fp'])==arch
 for k,v in e['crt_state'].items():assert state[k]==v
 return dict(status='matching58B arena allocation and real packing member receiver',candidate='_allocate_packing_memory',code_bytes=58,request=8,aligned_bytes=8,owned_memory_bytes=8,allocation_contents='actual untouched zero8 bytes from previously committed owned arena; allocator performs no zeroing',state_changes=['arena0.cursor+8','initializer publishes packing_stack to returned owned object'],member_formals=['this'],member_fields={'active':0,'free_frames':0},unvisited='overflow commit branch source/graph only',UNMODELED_initialized_bytes=4200,CRT_credit=0,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
