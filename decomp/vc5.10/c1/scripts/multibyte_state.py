"""Typed scanner-state and full genuine public-API transition proof."""
import copy,importlib.util,json,re,struct
from pathlib import Path
from provenance import sha,captured_native_binary
from multibyte_fields import defaults,read,seed,expected_fields
from allocation_state import symbols,call
from scaler_state import semantic

def validate_prefix(original,native,fields,state,sequence,probe):
 expected_fields(fields);current=state['compiler_state']['multibyte_state']
 assert current['offsets']==dict(classes=1836,flags=2092)and current['initialized_size']==2348
 assert read((original/'compiler-data-000.bin').read_bytes())==defaults()
 assert current['initialized']==read((original/('compiler-data-%03d.bin'%sequence)).read_bytes())
 saved=state['compiler_state']['bss']['saved_data']
 if saved:assert current['snapshot']==read((original/('compiler-snapshot-%03d.bin'%sequence)).read_bytes())
 else:assert current['snapshot']is None
 if probe=='restore':
  events=(original/'events.txt').read_text();observations=json.loads((native/'restore-observations.json').read_text())
  before=int(re.findall(r'^trap seq=(\d+) rva=000122a5 ',events,re.M)[0]);after=int(re.findall(r'^trap seq=(\d+) rva=000303d5 ',events,re.M)[0])
  assert len(re.findall(r'^scanner-state-seed fields=2 bytes=512 snapshot=[0-9a-f]+$',events,re.M))==1
  assert observations['pre_seed']['multibyte_state']['initialized']==defaults()
  for seq,phase,which in [(before,'before','live'),(after,'post_restore','snapshot')]:
   record=observations[phase]['multibyte_state']
   assert record['initialized']==read((original/('compiler-data-%03d.bin'%seq)).read_bytes())==seed(which)
   assert record['snapshot']==read((original/('compiler-snapshot-%03d.bin'%seq)).read_bytes())==seed('snapshot')
  assert current['initialized']==current['snapshot']==seed('snapshot')
 else:assert current['initialized']==current['snapshot']==defaults()

def validate(original,native,candidate,report,stage_state=None):
 project=Path(__file__).resolve().parents[1];fields=json.loads((project/candidate['field_map']).read_text());expected_fields(fields)
 following=stage_state is not None
 if following:
  assert candidate.get('path_fold_observers')and (candidate['expected_frontier']['kind'],candidate['expected_frontier']['rva'])in [('path-fold-complete',0x12f87),('environment-arguments-complete',0x11e3c)]
 else:assert candidate['expected_frontier']==dict(rva=0x12f3e,native_symbol='initialize_option_input_12f3e',kind='multibyte-scanner-complete',calling_convention='cdecl',native_args=[],caller=dict(native_symbol='parse_command_line',return_rva=0x12b80))
 spec=importlib.util.spec_from_file_location('own_original_locale',project/'scripts/check-isolated-locale.py');checker=importlib.util.module_from_spec(spec);spec.loader.exec_module(checker);own=checker.check(original)
 binary=captured_native_binary(native);names=symbols(binary);events=(original/'events.txt').read_text();module=0x10600000
 public=json.loads((native/'locale-native-setlocale-entry-state.json').read_text())
 from checksum_state import validate as validate_checksum,preserved
 prefix=validate_checksum(original,native,candidate,report,public_state=public)
 assert prefix['status']=='matching genuine public CRT entry; no locale behavior accepted'
 state=stage_state if following else json.loads((native/'state.json').read_text());records=json.loads((native/'locale-native-records.json').read_text())
 assert [row['phase']for row in records]==['before','after','mb-before','mb-after','next']
 assert [(row['kind'],row['name'])for row in records]==[('entry','setlocale'),('return','setlocale'),('entry','_setmbcp'),('return','_setmbcp'),('next','path-fold-entry-before-auxiliary'if following else'target-stub')]
 binding=json.loads((native/'isolated-locale-binding.json').read_text());assert binding['receipt_sha256']==sha(original/'receipt.json')
 native_names=('CRT-argv','CRT-environment','Invoke-argv');original_names=native_names
 from crt_vectors import vector,disjoint
 for index,row in enumerate(records):
  phase=row['phase'];actual=own['phases'][phase];w=actual['words']
  assert row['mb_cur_max']==w[1]and row['lc_codepage']==w[2]and row['lc_handles']==w[3:9]and row['mb_gate']==w[9]and row['mb_codepage']==w[10]
  assert row['errors']==actual['errors']==[0,0,0]and semantic(row['fp'])==actual['fp']
  assert row['compiler_state']==public['compiler_state']and row['invocation']==public['invocation']
  assert row['addresses']['mbctype']==names['recon_mbctype'][0]and names['recon_mbctype'][1]==257
  assert row['ctype_static']==names['character_flags'][0]and names['character_flags'][1]==514
  assert row['addresses']['old-ctype']==names['character_flags'][0]
  assert row['ctype_selection']==dict(bound=1,calls=0 if index==0 else 1,mb_calls=0 if index<3 else 1,reference=sha(original/'receipt.json'),result=row['addresses']['result'])
  for name in ('mbctype','ctype','old-ctype','result'):
   before=(original/('locale-'+phase+'-'+name+'.bin')).read_bytes();after=(native/('locale-native-'+phase+'-'+name+'.bin')).read_bytes()
   assert before==after
   assert row['addresses'][name]>0 or name=='result'and index==0
  if index==0:assert row['addresses']['ctype']==names['character_flags'][0]and row['addresses']['result']==0
  else:
   assert row['addresses']['ctype']!=names['character_flags'][0]and row['addresses']['result']
   assert row['addresses']==records[1]['addresses']
   spans=sorted([(row['addresses']['ctype'],row['addresses']['ctype']+514),(row['addresses']['old-ctype'],row['addresses']['old-ctype']+514),(row['addresses']['result'],row['addresses']['result']+len((native/('locale-native-'+phase+'-result.bin')).read_bytes()))])
   assert all(a[1]<=b[0]for a,b in zip(spans,spans[1:]))
  paths=[]
  for name in native_names:
   old=original/('locale-'+phase+'-'+name+'.bin');new=native/('locale-native-'+phase+'-'+name+'.bin');paths.append(new)
   old_root,old_values=vector(old);new_root,new_values=vector(new)
   assert [value for pointer,value in old_values]==[value for pointer,value in new_values]
   if name=='CRT-argv':assert row['crt_vectors']['argv']==dict(address=new_root,count=len(new_values))and len(new_values)==3
   elif name=='CRT-environment':assert row['crt_vectors']['environment']==dict(address=new_root,count=len(new_values))
   else:assert new_root==row['invocation']['args']['argv']and len(new_values)==11
  disjoint(paths)
  assert row['crt_vectors']['bound']==1 and row['crt_vectors']['argc']==3
  assert row['crt_state']==records[0]['crt_state']
  if row['kind']=='entry':
   symbol='recon_setlocale'if row['name']=='setlocale'else'recon_setmbcp';assert row['pc']==names[symbol][0]
   wanted=[dict(name='category',type='int'),dict(name='locale',type='const char *')]if index==0 else[dict(name='codepage',type='int')]
   assert row['formal_inventory']==wanted and len(row['words'])==len(wanted)+1
   call(binary,row['words'][0],row['pc'])
   assert row['caller']['pc']==row['words'][0]and row['caller']['symbol']==('parse_command_line'if index==0 else'initialize_multibyte_scanner')
   if index==0:assert row['words'][1:]==[2,names['c1_empty_string'][0]]and row['locale_bytes']==[0]
   else:assert row['words'][1:]==[0xfffffffc]and row['caller']['args']=={}
  elif row['kind']=='return':
   entry=records[index-1];assert row['pc']==entry['words'][0]and row['entry_esp']==entry['esp']and row['esp']==entry['esp']+4 and row['preserved']==entry['preserved']
   assert row['eax']==(row['addresses']['result']if index==1 else 0)
 assert records[0]['compiler_state']==records[-1]['compiler_state']==state['compiler_state']
 assert state['invocation']==public['invocation']and state['pass_mode']==public['pass_mode']
 assert semantic(state['command_line_fp'])==semantic(records[-1]['fp'])
 assert state['module_identity']==public['module_identity']and state['module_token']==public['module_token']
 for key,value in records[-1]['crt_state'].items():assert state[key]==value
 symbol='initialize_path_fold_table'if following else'initialize_option_input_12f3e'
 assert state['formal_inventory']==[]and state['args']=={}and state['symbol']==symbol
 assert state['caller']['symbol']=='parse_command_line'and state['caller']['args']==public['caller']['args']
 if following:assert state['pc']==names[symbol][0]
 else:
  assert names[symbol][0]<state['pc']<=sum(names[symbol])
  opcode=__import__('subprocess').check_output(['objdump','-d','--start-address='+str(state['pc']-1),'--stop-address='+str(state['pc']),str(binary)],text=True);assert'int3'in opcode
 call(binary,state['caller']['pc'],names[symbol][0])
 entries=json.loads((native/'scanner-init-entries.json').read_text());returns=json.loads((native/'scanner-init-returns.json').read_text());assert len(entries)==len(returns)==1
 entry,end=entries[0],returns[0];assert entry['pc']==names['initialize_multibyte_scanner'][0]and entry['formal_inventory']==[]and entry['caller']['symbol']=='parse_command_line'and entry['caller']['args']==public['caller']['args']
 assert entry['return']==entry['caller']['pc'];call(binary,entry['return'],entry['pc'])
 assert end['pc']==entry['return']and end['esp']==entry['esp']+4 and end['preserved']==entry['preserved']and end['compiler_state']==entry['compiler_state']==state['compiler_state']
 assert semantic(end['fp'])==semantic(entry['fp'])==semantic(records[-1]['fp'])
 seqs=re.findall(r'^trap seq=(\d+) rva=00012f08 esp=(\w+).*stack=([0-9a-f,]+)$',events,re.M);last=re.findall(r'^trap seq=(\d+) rva=00012f3e esp=(\w+).*stack=([0-9a-f,]+)$',events,re.M);assert len(seqs)==len(last)==1
 at,esp,words=seqs[0];next_at,next_esp,next_words=last[0];assert int(words.split(',')[0],16)==module+0x12b7b and int(next_words.split(',')[0],16)==module+0x12b80 and int(esp,16)==int(next_esp,16)
 assert preserved(events,int(at))==preserved(events,int(next_at))
 public_seq=int(re.findall(r'^public-entry seq=(\d+) module=MSVCRT.dll name=setlocale ',events,re.M)[0])
 assert (original/('state-%03d.bin'%int(next_at))).read_bytes()==(original/('state-%03d.bin'%public_seq)).read_bytes()
 for region in ('compiler-data','compiler-bss','compiler-snapshot'):
  boundary=(original/('%s-%03d.bin'%(region,int(next_at)))).read_bytes();before=(original/('locale-next-target-'+{'compiler-data':'data','compiler-bss':'BSS','compiler-snapshot':'snapshot'}[region]+'.bin')).read_bytes()
  if following and region=='compiler-bss'and json.loads((original/'receipt.json').read_text())['probe']=='restore':
   from path_fold_fields import RVA,SIZE,seed as fold_seed
   expected=bytearray(before);assert before[RVA-0x50000:RVA-0x50000+SIZE]==bytes(SIZE)
   expected[RVA-0x50000:RVA-0x50000+SIZE]=struct.pack('<256H',*fold_seed('helper'))
   assert boundary==bytes(expected),'only explicitly labeled known512B helper input seed intervenes'
  else:assert boundary==before
 probe=json.loads((original/'receipt.json').read_text())['probe'];validate_prefix(original,native,fields,state,int(next_at),probe)
 assert state['compiler_state']['multibyte_state']==entry['compiler_state']['multibyte_state']
 transcript=(native/'gdb.txt').read_text();assert'Program received signal SIGTRAP'in transcript and'recon_unsupported'not in transcript and'NativeLocaleEntry'in(native/'commands.gdb').read_text()
 grouped=None
 if(native/'grouped-equivalence.jsonl').exists():
  rows=[json.loads(line)for line in(native/'grouped-equivalence.jsonl').read_text().splitlines()];assert rows and all((row['equal']is True if row['legacy_seconds']is not None else row['equal']is None and row.get('mode')=='grouped-only')and not row['padding_semantics_claimed']for row in rows)
  for row in rows:
   for root,item in row['fresh_objects'].items():assert sha(native/item['path'])==item['sha256']and(native/item['path']).stat().st_size==item['bytes']
  grouped=dict(checkpoints=len(rows),new_seconds=sum(row['new_seconds']for row in rows),legacy_seconds=sum(row['legacy_seconds']for row in rows)if all(row['legacy_seconds']is not None for row in rows)else None,schema_sha256=sha(native/'grouped-layout.json'))
 return dict(status='matching complete scanner and isolated public CRT transitions; supported SBCS1252 case only',native_symbol=state['symbol'],application_candidate='_initialize_multibyte_scanner',candidate_code_bytes=72,initialized_known_bytes=2348,initialized_unmodeled_bytes=4200,scanner_table_bytes=512,CRT_code_credit=0,api_calls=['setlocale(2,empty)','_setmbcp(-4)'],errors=[0,0,0],FP_unchanged=True,private_vector_contents_equal=True,grouped_decoder=grouped,execution_receipts=dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json')))
