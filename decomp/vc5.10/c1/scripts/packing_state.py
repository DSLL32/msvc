"""Packing initializer reaches its real allocation before any state write."""
import json,re,struct,subprocess
from provenance import captured_native_binary,sha
from allocation_state import symbols,call
from vector_state import snapshot
from integer_state import load,locale_raw
from normalizer_state import validate as validate_prefix
from scaler_state import semantic
from signature_call_state import fp
from frontier_contract import validate_original_frontier,validate_native_stop
BASE=0x10600000

def validate(original,native,candidate,report,stage_state=None):
 entries=load(native,'packing-entries');assert len(entries)==1
 stage=snapshot(original,native,load(native,'state'),entries[0])
 prefix=validate_prefix(original,native,candidate,report,stage_state=stage)
 result=validate_endpoint(original,native,candidate,stage_state=stage_state);result['prefix']=prefix;return result

def validate_endpoint(original,native,candidate,stage_state=None):
 assert candidate['packing_observers']==dict(entry_rva=0x1348d,allocator_rva=0x134c9,return_rva=0x13494,request_bytes=8)
 expected=dict(kind='packing-initializer-allocator',rva=0x134c9,native_symbol='allocate_packing_memory',native_args=['bytes'],calling_convention='cdecl',caller=dict(native_symbol='initialize_packing',return_rva=0x13494))
 assert candidate['expected_frontier']==expected if stage_state is None else candidate['expected_frontier']['kind']in ('packing-allocation-complete','packing-frame-complete')
 assert candidate['allowed_probes']==['normal']
 f=[r for r in candidate['functions']if r['name']=='_initialize_packing'];assert len(f)==1 and f[0]['chunks']==[[0x1348d,0x134c9],[0x30af8,0x30b02]]and f[0]['role']=='application'
 entries=load(native,'packing-entries');assert len(entries)==1;entry=entries[0];state=load(native,'state')if stage_state is None else stage_state
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,'normal');validate_native_stop(native,candidate)
 def event(rva):
  rows=re.findall(r'^trap seq=(\d+) rva=%08x esp=(\w+).*stack=([\w,]+)$'%rva,events,re.M);assert len(rows)==1
  return int(rows[0][0]),int(rows[0][1],16),[int(v,16)for v in rows[0][2].split(',')]
 seq,esp,words=event(0x1348d);end_seq,end_esp,end_words=event(0x134c9)
 assert words[0]==BASE+0x12d55 and end_esp==esp-8 and end_words[:3]==[BASE+0x13494,8,BASE+0x12d55]
 for stem in ('compiler-data','compiler-bss','compiler-snapshot','state'):
  assert(original/('%s-%03d.bin'%(stem,seq))).read_bytes()==(original/('%s-%03d.bin'%(stem,end_seq))).read_bytes()
 assert locale_raw(original,'packing-entry')==locale_raw(original,'packing-allocator')
 for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv','target-data','target-BSS','target-snapshot'):
  assert(original/('locale-packing-entry-'+suffix+'.bin')).read_bytes()==(original/('locale-packing-allocator-'+suffix+'.bin')).read_bytes()
 binary=captured_native_binary(native);names=symbols(binary)
 assert entry['pc']==names['initialize_packing'][0]and entry['formal_inventory']==[]and entry['args']=={}and entry['words']==[entry['caller']['pc']]
 assert state['symbol']=='allocate_packing_memory'and state['args']=={'bytes':8}and state['formal_inventory']==[dict(name='bytes',type='unsigned int')]
 assert names['allocate_packing_memory'][0]<=state['pc']<=sum(names['allocate_packing_memory'])
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert ('int3'in opcode if stage_state is None else state['pc']==names['allocate_packing_memory'][0])
 assert state['caller']['symbol']=='initialize_packing'and state['caller']['args']=={}and set(state['caller'])=={'symbol','args','pc'}
 # The independently observed physical caller entry inventories all zero
 # formals; generic final caller records carry only symbol/args/return PC.
 call(binary,state['caller']['pc'],names['allocate_packing_memory'][0]);assert state['compiler_state']==entry['compiler_state']and state['invocation']==entry['invocation']
 for k,v in entry['crt_state'].items():assert state[k]==v
 assert semantic(state['command_line_fp'])==semantic(entry['fp'])==fp((original/'locale-packing-entry-fp.bin').read_bytes())
 assert entry['errors']==[0,0,0]and entry['locale']['errors']==[0,0,0]
 assert not entry['compiler_state']['option_state']['initialized']['packing_stack']
 return dict(status='matching70B packing initializer to genuine allocator8 frontier',candidate='_initialize_packing',code_bytes=70,first_call_request_bytes=8,state_writes_before_frontier=0,UNMODELED_initialized_bytes=4200,unknown_consumers='none before allocator; pending members remain named INT3',mode='normal only',CRT_credit=0,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
