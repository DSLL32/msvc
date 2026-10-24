"""Complete source-owned path folding with a qualified public CRT boundary."""
import copy,json,re,struct
from provenance import captured_native_binary,sha
from path_fold_original import check as check_original,record
from path_fold_fields import expected_fields,read,seed,RVA,SIZE
from allocation_state import symbols,call
from scaler_state import semantic,original_fp

def validate(original,native,candidate,report):
 from provenance import PROJECT
 fields=json.loads((PROJECT/candidate['field_map']).read_text());expected_fields(fields)
 own=check_original(original);binary=captured_native_binary(native);names=symbols(binary)
 entries=json.loads((native/'path-fold-entries.json').read_text());returns=json.loads((native/'path-fold-returns.json').read_text());calls=json.loads((native/'path-fold-calls.json').read_text())
 assert len(entries)==len(returns)==1 and len(calls)==own['calls']
 entry,end=entries[0],returns[0];state=json.loads((native/'state.json').read_text())
 assert entry['pc']==names['initialize_path_fold_table'][0]and entry['symbol']=='initialize_path_fold_table'
 assert entry['formal_inventory']==[]and entry['return']==entry['caller']['pc']and entry['caller']['symbol']=='parse_command_line'
 call(binary,entry['return'],entry['pc'])
 assert entry['pre_seed']['path_fold']==[0]*256
 assert entry['compiler_state']['path_fold']==(seed('helper')if own['probe']=='restore'else[0]*256)
 wanted=copy.deepcopy(entry['pre_seed']);wanted['path_fold']=entry['compiler_state']['path_fold'];assert wanted==entry['compiler_state']
 public=json.loads((native/'locale-native-setlocale-entry-state.json').read_text())
 stage=copy.deepcopy(public);stage.update(symbol=entry['symbol'],pc=entry['pc'],args={},formal_inventory=entry['formal_inventory'],caller=entry['caller'],compiler_state=entry['pre_seed'],command_line_fp=entry['fp'],pass_mode=entry['pre_seed']['pass_mode'],invocation=entry['invocation']);stage.update(entry['crt_state'])
 from multibyte_state import validate as validate_scanner
 prefix=validate_scanner(original,native,candidate,report,stage_state=stage)
 assert prefix['CRT_code_credit']==0
 events=(original/'events.txt').read_text()
 returned_sequence=int(re.findall(r'^path-fold-helper-return seq=(\d+) ',events,re.M)[0])
 original_floating=semantic(original_fp(original,returned_sequence))
 for index,row in enumerate(calls):
  a,b=row['entry'],row['return'];raw_a=record(original/('path-fold-%03d-entry.bin'%index));raw_b=record(original/('path-fold-%03d-return.bin'%index));character,value=own['results'][index]
  assert a['kind']=='entry'and a['pc']==names['recon_mbctolower'][0]and a['formal_inventory']==[dict(name='character',type='unsigned int')]
  assert len(a['words'])==2 and a['words'][1]==character and a['caller']==dict(symbol='initialize_path_fold_table',pc=a['words'][0])
  call(binary,a['words'][0],a['pc'])
  assert b['kind']=='return'and b['pc']==a['words'][0]and b['esp']==a['esp']+4 and b['entry_esp']==a['esp']and b['eax']==value and b['preserved']==a['preserved']
  for native_row,raw in ((a,raw_a),(b,raw_b)):
   w=raw['state'];assert native_row['errors']==raw['meta'][12:15]==[0,0,0]
   assert native_row['mb_cur_max']==w[1]and native_row['lc_codepage']==w[2]and native_row['lc_handles']==w[3:9]and native_row['mb_gate']==w[9]and native_row['mb_codepage']==w[10]
   assert native_row['mb']==list(raw['mb'])and native_row['ctype']==list(raw['ctype'])and native_row['old_ctype']==list(raw['old_ctype'])and native_row['classes']==list(raw['classes'])
   assert native_row['result']==list(raw['result'].split(b'\0',1)[0]+b'\0')and native_row['fold']==raw['fold']
   assert native_row['addresses']==calls[0]['entry']['addresses']
   assert native_row['selection']==dict(bound=1,calls=1,mb_calls=1,reference=sha(original/'receipt.json'),result=native_row['addresses']['result'])
   assert semantic(native_row['fp'])==original_floating
 assert end['pc']==entry['return']and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']
 assert semantic(end['fp'])==semantic(entry['fp'])==semantic(state['command_line_fp'])
 table=read((original/'path-fold-complete-target-BSS.bin').read_bytes());wanted=copy.deepcopy(entry['compiler_state']);wanted['path_fold']=table
 assert end['compiler_state']==state['compiler_state']==wanted
 assert state['symbol']=='read_env_arguments'and state['args']==dict(arguments=0)and state['formal_inventory']==[dict(name='arguments',type='char *')]
 assert names['read_env_arguments'][0]<state['pc']<=sum(names['read_env_arguments'])
 assert state['caller']['symbol']=='parse_command_line'and state['caller']['args']==entry['caller']['args'];call(binary,state['caller']['pc'],names['read_env_arguments'][0])
 transcript=(native/'gdb.txt').read_text();assert'Program received signal SIGTRAP'in transcript and'recon_unsupported'not in transcript
 assert state['invocation']==entry['invocation']and state['pass_mode']==entry['compiler_state']['pass_mode']
 for key,value in entry['crt_state'].items():assert end['crt_state'][key]==state[key]==value
 if own['probe']=='restore':
  observations=json.loads((native/'restore-observations.json').read_text());assert observations['pre_seed']['path_fold']==[0]*256 and observations['before']['path_fold']==seed('reset')and observations['post_restore']['path_fold']==[0]*256
 grouped=prefix['grouped_decoder'];assert grouped and grouped['legacy_seconds']is None
 return dict(status='matching complete path-fold initializer and genuine qualified CRT calls',application_candidate='_initialize_path_fold_table',candidate_code_bytes=83,path_table_bytes=512,initialized_known_bytes=2348,initialized_unmodeled_bytes=4200,native_bss_bytes=2372,public_CRT_calls=own['calls'],CRT_code_credit=0,grouped_decoder=grouped,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
