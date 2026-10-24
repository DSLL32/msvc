"""Known normal path copied to a real owned local buffer before its helper."""
import json,re,struct,subprocess
from integer_state import validate as validate_integer,load
from vector_state import snapshot
from provenance import captured_native_binary,sha
from allocation_state import symbols,call
from scaler_state import semantic
from signature_call_state import fp
from frontier_contract import validate_original_frontier,validate_native_stop

BASE=0x10600000
VALUE=b'vc50.pdb\0'
FORMALS=[dict(name=k,type=t)for k,t in [('output','char *'),('input','const char *'),('size','unsigned int')]]

def validate(original,native,candidate,report):
 assert candidate['allowed_probes']==['normal']and load(original,'receipt')['probe']==load(native,'receipt')['probe']=='normal'
 assert candidate['expected_frontier']==dict(kind='option-path-normalizer',rva=0x1344d,native_symbol='normalize_path',native_args=['path'],calling_convention='cdecl',caller=dict(native_symbol='resolve_option_path',return_rva=0x133c9))
 assert candidate['resolver_observers']==dict(entry_rva=0x1335b,frontier_rva=0x1344d,input='vc50.pdb',input_bytes=9,local_buffer_bytes=260,output_buffer_bytes=520)
 functions=[r for r in candidate['functions']if r['name']=='_resolve_option_path'];assert len(functions)==1 and functions[0]['chunks']==[[0x1335b,0x1344d],[0x30a3c,0x30af0]]and functions[0]['role']=='application'
 rows=load(native,'resolver-entries');assert len(rows)==1;entry=rows[0];state=load(native,'state')
 stage=snapshot(original,native,state,entry);prefix=validate_integer(original,native,candidate,report,stage_state=stage)
 result=validate_copy(original,native,candidate,entry,state);result['prefix']=prefix;return result

def validate_copy(original,native,candidate,entry=None,state=None):
 assert candidate['allowed_probes']==['normal']and load(original,'receipt')['probe']==load(native,'receipt')['probe']=='normal'
 assert candidate['expected_frontier']['kind']=='option-path-normalizer'and candidate['expected_frontier']['rva']==0x1344d
 rows=load(native,'resolver-entries');assert len(rows)==1;entry=rows[0]if entry is None else entry;state=load(native,'state')if state is None else state
 events=(original/'events.txt').read_text();validate_original_frontier(events,candidate,'normal');validate_native_stop(native,candidate)
 r=list(struct.unpack('<16I',(original/'resolver-entry-registers.bin').read_bytes()))
 assert r[1]==BASE+0x1335b and r[10]==BASE+0x12c8b and r[11]==r[2]+32 and r[13:]==[520,BASE,1]
 actual=re.findall(r'^trap seq=(\d+) rva=0001335b esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(actual)==1
 assert int(actual[0][0])==r[0]and int(actual[0][1],16)==r[2]and [int(v,16)for v in actual[0][2].split(',')][:4]==r[10:14]
 assert(original/'resolver-entry-input.bin').read_bytes()==VALUE and entry['input_bytes']==list(VALUE)
 assert entry['symbol']=='resolve_option_path'and entry['formal_inventory']==FORMALS and set(entry['args'])=={'output','input','size'}and entry['words']==[entry['caller']['pc']]+[entry['args'][k]for k in ('output','input','size')]
 assert entry['args']['size']==520 and entry['args']['input']==entry['compiler_state']['option_state']['initialized']['database_file']
 rows=re.findall(r'^trap seq=(\d+) rva=0001344d esp=(\w+).*stack=([\w,]+)$',events,re.M);assert len(rows)==1
 seq=int(rows[0][0]);esp=int(rows[0][1],16);words=[int(v,16)for v in rows[0][2].split(',')]
 assert esp==r[2]-284 and words[:2]==[BASE+0x133c9,r[2]-260]
 assert(original/'resolver-normalize-input.bin').read_bytes()==VALUE
 assert state['symbol']=='normalize_path'and state['formal_inventory']==[dict(name='path',type='char *')]and set(state['args'])=={'path'}
 assert state['caller']['symbol']=='resolve_option_path'and state['caller']['formal_inventory']==FORMALS and state['caller']['args']==entry['args']
 local=state['resolver_local_buffer'];assert local['bytes']==260 and local['address']==state['args']['path']and state['resolver_path_bytes']==list(VALUE)
 assert local['address']+260<=entry['args']['output']or entry['args']['output']+520<=local['address']
 assert not local['address']<=entry['args']['input']<local['address']+260
 binary=captured_native_binary(native);names=symbols(binary);assert entry['pc']==names['resolve_option_path'][0]
 assert names['normalize_path'][0]<state['pc']<=sum(names['normalize_path'])
 call(binary,state['caller']['pc'],names['normalize_path'][0])
 opcode=subprocess.check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert 'int3'in opcode
 assert entry['compiler_state']==state['compiler_state']and entry['invocation']==state['invocation']and entry['module_identity']==state['module_identity']and entry['module_token']==state['module_token']and state['pass_mode']==entry['compiler_state']['pass_mode']
 for key,value in entry['crt_state'].items():assert state[key]==value
 assert(original/('state-%03d.bin'%r[0])).read_bytes()==(original/('state-%03d.bin'%seq)).read_bytes()
 for suffix in ('words','errors','fp','mbctype','ctype','old-ctype','result','CRT-argv','CRT-environment','Invoke-argv','target-data','target-BSS','target-snapshot'):
  assert(original/('locale-resolver-entry-'+suffix+'.bin')).read_bytes()==(original/('locale-resolver-normalize-'+suffix+'.bin')).read_bytes()
 original_fp=fp((original/'locale-resolver-entry-fp.bin').read_bytes());assert semantic(entry['fp'])==semantic(state['command_line_fp'])==original_fp
 assert entry['errors']==[0,0,0]and entry['locale']['errors']==state['resolver_locale']['errors']==[0,0,0]
 assert {k:v for k,v in entry['locale'].items()if k not in('phase','fp')}=={k:v for k,v in state['resolver_locale'].items()if k not in('phase','fp')}
 assert entry['locale']['fp']==state['resolver_locale']['fp']
 return dict(status='matching422B resolver to actual first normalize_path frontier',application_candidate='_resolve_option_path',candidate_code_bytes=422,mode='normal only',defined_input='vc50.pdb NUL9',local_buffer_bytes=260,defined_copied_bytes=9,uninitialized_local_tail='251 bytes unclaimed',output_buffer_bytes=520,output_contents='not read or claimed',whole_target_state_unchanged=True,helper_formals=['path'],import_calls_reached=[],UNMODELED_initialized_bytes=4200,UNMODELED_literal_tail_bytes=16,restore='old opaque input remains held at1335b in its own frozen configuration/ELF',unvisited='rooted/UNC/lead-byte/dot-component/_fullpath/fallback/error branches source+graph only',CRT_credit=0,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
