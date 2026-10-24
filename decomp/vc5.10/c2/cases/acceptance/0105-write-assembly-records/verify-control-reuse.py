from pathlib import Path
import json,sys,time,hashlib,copy,struct
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from compare import compare
from state import sha
start=time.monotonic();root=S/'numeric-control-flow';oldO=root/'assembly105-original';oldN=root/'assembly105-native';newO=root/'assembly105-original-final';newN=root/'assembly105-native-final'
old=[json.loads((p/'receipt.json').read_text())for p in [oldO,oldN]];new=[json.loads((p/'receipt.json').read_text())for p in [newO,newN]]
keys=[['target_sha256','case_id','case_inputs','child_cwd','executable_sha256','debugger_sha256','provider_sha256','environment','source_hashes','candidates','helper_inputs_sha256','execution_inputs'],['native_sha256','binary_snapshot_sha256','case_id','case_inputs','environment','source_hashes','candidates','provenance_sha256','target_environment_sha256']]
for index in range(2):
 for key in keys[index]:assert old[index][key]==new[index][key],key
before=json.loads((oldO/'base_configuration.json').read_text());after=json.loads((newO/'base_configuration.json').read_text());row=lambda c:next(r for r in c['stubs']if r['name']=='@lookup_initializer@4');a=row(before);b=row(after);old_abi=a['abi'];new_abi=b['abi'];a['abi']=b['abi'];assert before==after,'only erroneous non-executable lookup ABI prose correction permitted'
assert set(old[1]['gdb_inputs'])==set(new[1]['gdb_inputs'])
for name,digest in old[1]['gdb_inputs'].items():
    previous=(oldN/name).read_bytes();current=(newN/name).read_bytes()
    assert sha(previous)==digest and sha(current)==new[1]['gdb_inputs'][name]
    assert previous.replace(str(oldN).encode(),str(newN).encode()).replace(old_abi.encode(),new_abi.encode())==current,'observer change outside exact output-directory and non-executable metadata prose'
for field in ['function_input','globals_complete','globals','trace','function']:
 a=json.loads((oldN/'state.json').read_text())[field];b=json.loads((newN/'state.json').read_text())[field]
 if field=='globals_complete':
  for v in [a,b]:
   raw=bytes.fromhex(v['streams']['input_bytes']);words=list(struct.unpack('<8I',raw));assert words[0]==words[2]and words[2]!=0 and words[7]==0
   words[0]=0x10000000;words[2]=0x10000000;v['streams']['input_bytes']=struct.pack('<8I',*words).hex()
 assert a==b,field
previous=json.loads((S/'assembly105-input-controls.json').read_text());assert previous['checker_sha256']==sha((B/'scripts/check-function-input.py').read_bytes())and previous['count']==16
assert previous['original_receipt_sha256']==sha((oldO/'receipt.json').read_bytes())and previous['native_receipt_sha256']==sha((oldN/'receipt.json').read_bytes())
positive=compare(newO,newN)
value={'source_control_result':str(S/'assembly105-input-controls.json'),'source_control_result_sha256':sha((S/'assembly105-input-controls.json').read_bytes()),'reused_count':16,'fresh_current_positive':positive,'seconds':time.monotonic()-start,'bound_unchanged_original_keys':keys[0],'bound_unchanged_native_keys':keys[1],'exact_native_state_fields':['function_input','globals_complete','globals','trace','function'],'stdio_logical_normalization':'Only known ShimFILE input pointer/base words0/2 normalized together after actualptr==base nonNULL and tmpnameNULL; counters/flags/index/bufsize/validbytes unchanged and freshpositive validates each actualFILE. Other function_input/globals_complete/controller fields exact.',
'observer_transport_equivalence':'Exact per-file bytes after only old/new owned output-directory literal and same non-executable ABI-prose substitution; actual old/new GDB input hashes verified, no executable observer logic changes',
'only_config_change':'non-executable lookup ABI prose: existing c2_report_internal is extent-known/unaccepted, not accepted; complete target/mask/helper/source/binary/inputs/state/observer unchanged','current_receipts':{str(p/'receipt.json'):sha((p/'receipt.json').read_bytes())for p in [newO,newN]},'checker_sha256':sha((B/'scripts/check-function-input.py').read_bytes()),'validator_sha256':sha((B/'scripts/compare.py').read_bytes()),'model_sha256':sha((B/'scripts/globals_state.py').read_bytes())}
(S/'assembly105-input-control-reuse.json').write_text(json.dumps(value,indent=2)+'\n')
