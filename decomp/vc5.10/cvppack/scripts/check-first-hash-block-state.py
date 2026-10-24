#!/usr/bin/env python3
"""Finite accessor/live-block/564-byte copy controls against production predicates."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0112-first-hash-block'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=module('first_block_raw','raw-state.py');controller=module('first_block_controller','address-hash-complete-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native-generation/state.json').read_text());artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes()for p in (pair/folder).glob(pattern)}for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native-generation','native-view-*.bin')]}
full=raw.compare(config,data,events,native,artifacts,views)
phase_events,previous,phase=controller.first_phase(events,native);historical=raw.decode(phase_events);lines=events.splitlines();begin=lines.index('addresshash-phase-entry 0040edc0');end=lines.index('addresshash-phase-end 0040edc0');original=raw.decode('\n'.join(lines[:begin]+[line for line in lines[begin:end+1]if line.startswith('mapped-view ')]+lines[end+1:])+'\n');observations=controller.decode(events)
def helper(a,b,c):return controller.first_block(config,a,b,historical,b['address_hash_first_callback'],c,b['address_hash_complete'])
assert helper(original,native,observations)==full['first_block'];negatives=[]
def reject(name,mutate,reason):
 a=copy.deepcopy(original);b=copy.deepcopy(native);c=copy.deepcopy(observations);mutate(a,b,c)
 try:helper(a,b,c)
 except (AssertionError,KeyError)as error:
  assert reason in str(error),(name,str(error));negatives.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-original-caller-return',lambda a,b,c:c['first_block'].update(return_pc=0),'caller/formal')
reject('wrong-native-caller-owner',lambda a,b,c:b['address_hash_complete']['first_block'].update(owner='write_globals'),'caller/formal')
reject('wrong-native-local-arena',lambda a,b,c:b['address_hash_complete']['first_block'].update(local='addresses'),'caller/formal')
reject('wrong-native-actual-input-formal',lambda a,b,c:b['address_hash_complete']['first_block'].update(formal_type='unsigned int *'),'caller/formal')
reject('wrong-input-arena-identity',lambda a,b,c:b['address_hash_complete']['first_block'].update(argument=0),'live arena')
reject('changed-original-after-input',lambda a,b,c:c['first_block'].update(after_arena=bytes(24)),'input preservation')
reject('changed-native-after-input',lambda a,b,c:b['address_hash_complete']['first_block'].update(after_arena='00'*24),'input preservation')
reject('wrong-returned-live-block',lambda a,b,c:b['address_hash_complete']['first_block'].update(result=0),'result/block identity')
reject('changed-returned-block-bytes',lambda a,b,c:b['address_hash_complete']['first_block'].update(block='00'*12),'result/block identity')
reject('changed-EDI-on-return',lambda a,b,c:b['address_hash_complete']['first_block']['after_nonvolatile'].__setitem__(2,0),'nonvolatile ABI')
reject('wrong-original-RET-cleanup',lambda a,b,c:c['first_block'].update(exit_esp=0),'original cdecl')
reject('wrong-original-caller-result',lambda a,b,c:c['first_block'].update(caller_result=0),'original cdecl')
reject('wrong-native-RET-cleanup',lambda a,b,c:b['address_hash_complete']['first_block'].update(exit_esp=0),'native cdecl')
for field in ['copy_esp','copy_block','copy_source_end','copy_destination_end']:
 reject('wrong-actual-inline-'+field,lambda a,b,c,field=field:c['first_block'].update({field:0}),'inline copy boundary')
reject('wrong-next-original-block-alias',lambda a,b,c:a.update(stack=(a['stack'][0],0,*a['stack'][2:])),'next actual live block')
reject('wrong-next-original-block-contents',lambda a,b,c:a['memory'].update({historical['name_hash_complete']['block_address']:bytes(12)}),'next actual live block')
reject('missing-actual-next-formal',lambda a,b,c:b['observed_argument_types'].clear(),'next formal ABI')
reject('wrong-actual-next-pointee-width',lambda a,b,c:b['observed_argument_types'][0].update(pointee_width=4),'next formal ABI')
reject('wrong-next-current-local-owner',lambda a,b,c:b['parameters']['block'].update(local='names'),'local-pointer value')
reject('wrong-native-return-instruction',lambda a,b,c:b['address_hash_complete']['first_block'].update(return_pc=1),'genuine caller')
# Raw input/result mutation is decoded through production, not cached JSON.
line=next(l for l in events.splitlines()if l.startswith('first-block-result '));parts=line.split();parts[-1]='00'*12;changed=controller.decode(events.replace(line,' '.join(parts),1))
try:helper(original,native,changed)
except AssertionError as error:assert 'result/block identity' in str(error);negatives.append(dict(name='actual-raw-result-block-mutation',diagnostic=str(error)))
else:raise AssertionError('accepted raw result mutation')
# One full current-state check covers the newly copied output bytes; no repeat
# of the prior77 unrelated sort/stream/lifetime control campaign.
b=copy.deepcopy(native);value=bytearray.fromhex(b['output_buffer_state']['bytes']);value[7751]^=1;b['output_buffer_state']['bytes']=value.hex()
try:controller.validate_current(config,data,original,b,historical,previous,observations,b['address_hash_complete'],full['prefix'],full['fresh_historical_phase'],controller.prove(config))
except AssertionError as error:assert 'complete packed-output extent' in str(error);negatives.append(dict(name='changed-last-newly-copied-output-byte',diagnostic=str(error)))
else:raise AssertionError('accepted changed copied byte')
result=dict(schema=1,native_only=True,fresh_negative_count=len(negatives),rejected=negatives,positive=full['first_block'],seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/address-hash-complete-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native-generation/receipt.json'),reused_prior_controls=dict(path='cases/hello-debug-dll/evidence/0111-address-comparator-output/complete-address-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0111-address-comparator-output/complete-address-checks.json'),count=77,qualification='Prior complete-sort/address-stream/lifetime predicates retain exact111 branch;112 adds helper branch and expected names copy, same raw bounded first-callback prefix predicates run fresh. Historical77receipt/source snapshots retained, not relabeled as fresh. Current helper/formal/copy controls are new.'))
(pair/'first-block-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(controls=len(negatives),seconds=result['seconds'])))
