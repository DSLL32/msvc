#!/usr/bin/env python3
"""Focused genuine chain-return/dead-frame/live-owner controls; native only."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0113-next-hash-block-observed'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=module('hash_chain_raw','raw-state.py');chain=module('hash_chain_controller','hash-chain-state.py');complete=chain.complete_module()
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text());artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes()for p in (pair/folder).glob(pattern)}for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native','native-view-*.bin')]}
full=raw.compare(config,data,events,native,artifacts,views)
phase_events,previous,proof=chain.phase(events,native);lines=phase_events.splitlines();a=lines.index('addresshash-phase-entry 0040edc0');b=lines.index('addresshash-phase-end 0040edc0');historical=raw.decode('\n'.join(lines[:a]+[line for line in lines[a:b+1]if line.startswith('mapped-view ')]+lines[b+1:])+'\n')
current=[];inside=False
for line in events.splitlines():
 if line.startswith(('addresshash-phase-entry ','hash-next-phase-entry ')):inside=True;continue
 if line.startswith(('addresshash-phase-end ','hash-next-phase-end ')):inside=False;continue
 if not inside or line.startswith('mapped-view '):current.append(line)
original=raw.decode('\n'.join(current)+'\n');observations=chain.decode(events);observations['address_phase']=complete.decode(phase_events)['current'];policy=chain.prove(config)
def compare(a,b,c):return chain.validate_current(config,data,a,b,historical,b['hash_chain_entry_phase'],c,b['hash_chain'],full['prefix'],proof,policy)
assert compare(original,native,observations)==full
rejected=[]
def reject(name,mutate,reason):
 a=copy.deepcopy(original);b=copy.deepcopy(native);c=copy.deepcopy(observations);mutate(a,b,c)
 try:compare(a,b,c)
 except (AssertionError,KeyError)as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-final-frontier',lambda a,b,c:a.update(address=0x4100d0),'final real frontier')
reject('missing-genuine-helper',lambda a,b,c:b['hash_chain']['calls'].pop(),'helper inventory')
reject('wrong-actual-original-checkpoint',lambda a,b,c:c['calls'][1].update(site=0x4100c0),'sequence/ABI')
reject('wrong-actual-original-EAX',lambda a,b,c:c['calls'][1].update(observed_eax=0),'argument-load EAX')
reject('wrong-original-RET-site',lambda a,b,c:c['calls'][2].update(return_site=0),'sequence/ABI')
reject('wrong-original-caller-return',lambda a,b,c:c['calls'][2].update(return_pc=0),'sequence/ABI')
reject('wrong-native-actual-formal',lambda a,b,c:b['hash_chain']['calls'][1].update(formal_type='TypeArenaBlock *'),'sequence/ABI')
reject('wrong-live-argument-generation',lambda a,b,c:b['hash_chain']['calls'][2].update(argument=0),'live input/alias')
reject('changed-after-input-bytes',lambda a,b,c:b['hash_chain']['calls'][1].update(after_input='00'*24),'live input/alias')
reject('wrong-next-result-nonnull',lambda a,b,c:b['hash_chain']['calls'][2].update(result=1),'actual result')
reject('wrong-first-result-address-owner',lambda a,b,c:b['hash_chain']['calls'][1].update(result=0),'actual result')
reject('wrong-native-CALL-return-role',lambda a,b,c:b['hash_chain']['calls'][1].update(return_pc=1),'actual CALL role')
reject('wrong-native-entry-symbol',lambda a,b,c:b['hash_chain']['calls'][2].update(entry_pc=1),'actual CALL role')
reject('wrong-original-RET-cleanup',lambda a,b,c:c['calls'][0].update(exit_esp=0),'return ABI')
reject('wrong-native-RET-cleanup',lambda a,b,c:b['hash_chain']['calls'][0].update(exit_esp=0),'return ABI')
reject('changed-EDI-on-return',lambda a,b,c:b['hash_chain']['calls'][2]['after_nonvolatile'].__setitem__(2,0),'return ABI')
reject('wrong-inline-copy-ESP',lambda a,b,c:c['copy'].__setitem__(0,0),'inline-copy stack')
reject('wrong-inline-copy-source-end',lambda a,b,c:c['copy'].__setitem__(2,0),'address copy boundary')
reject('wrong-final-name-block-owner',lambda a,b,c:b['hash_chain']['current']['names'].update(block_address=0),'current live heap')
reject('changed-final-address-payload',lambda a,b,c:b['hash_chain']['current']['addresses'].update(payload='00'*540),'current live heap')
reject('changed-final-name-block-link',lambda a,b,c:b['hash_chain']['current']['names'].update(block='00'*12),'current live heap')
def change_output(b,at):
 value=bytearray.fromhex(b['output_buffer_state']['bytes']);value[at]^=1;b['output_buffer_state']['bytes']=value.hex()
reject('changed-new-address-last-byte',lambda a,b,c:change_output(b,8291),'complete packed output')
reject('changed-carried-names-last-byte',lambda a,b,c:change_output(b,7751),'complete packed output')
reject('wrong-output-defined-extent',lambda a,b,c:b['output_buffer_state'].update(used=7752),'complete packed output')
reject('wrong-current-directory-count',lambda a,b,c:b['globals'].update(directory_count='3a000000'),'changed scalar')
reject('wrong-current-image-position',lambda a,b,c:b['globals'].update(image_read_mode='00000000'),'changed scalar')
reject('wrong-next-output-scalar',lambda a,b,c:b['parameters'].update(offset=7752),'next typed output arguments')
reject('wrong-next-output-owner-index',lambda a,b,c:b['parameters']['entry'].update(index=58),'next typed output arguments')
reject('wrong-next-actual-DWORD-width',lambda a,b,c:b['observed_argument_types'][1].update(width=2),'next typed output arguments')
reject('unknown-new-allocation-event',lambda a,b,c:b['cleared_allocations'].append(dict(address=1,size=4)),'lifecycle/API continuation')
reject('unknown-late-mapping-call',lambda a,b,c:b['mapping_calls'].append(dict(unknown=True)),'lifecycle/API continuation')
reject('dead-parent-frame-relabeled-current',lambda a,b,c:b.update(public_writer_frame={}), 'dead frame relabeled')
reject('current-generic-retired-range-crossing',lambda a,b,c:b['memory'].update({str(b['main_free_calls'][7]['address']-4):'00'*8}),'generic objects changed/leaked')
reject('current-generic-own-unwritten-tail-read',lambda a,b,c:b['memory'].update({str(b['hash_chain']['current']['addresses']['payload_address']+4095):'00'}),'generic objects changed/leaked')
reject('changed-own-LastError',lambda a,b,c:b['output_buffer_state']['environment'].update(last_error=77),'own full FP/error')
reject('changed-own-unused-FP-payload',lambda a,b,c:b['output_buffer_state']['environment']['fp'].update(st7='0x'+'ff'*10),'own full FP/error')
for name,mutate,reason in [('wrong-typed-checkpoint-PC',lambda b:b['hash_chain']['phase'].update(typed_pc=0),'physical/typed phase'),('wrong-typed-checkpoint-ESP',lambda b:b['hash_chain']['phase'].update(physical_esp=0),'physical/typed phase')]:
 b=copy.deepcopy(native);mutate(b)
 try:chain.phase(events,b)
 except AssertionError as error:assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
line=next(l for l in events.splitlines()if l.startswith('hash-chain-call 1 '));parts=line.split();parts[6]='00000000';changed=chain.decode(events.replace(line,' '.join(parts),1));changed['address_phase']=observations['address_phase']
try:compare(original,native,changed)
except AssertionError as error:assert 'argument-load EAX'in str(error);rejected.append(dict(name='raw-mutated-observed-EAX',diagnostic=str(error)))
else:raise AssertionError('accepted raw EAX mutation')
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,seconds=time.perf_counter()-start,positive=full['hash_chain'],checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/hash-chain-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused_prior_controls=dict(path='cases/hello-debug-dll/evidence/0112-first-hash-block/first-block-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0112-first-hash-block/first-block-checks.json'),count=25,qualification='Unchanged112/111 predicates are freshly run on actual113 candidate-entry historical phase. Prior25/77 campaigns retain exact historical scopes; current helper/dead-frame/live-current-state controls are fresh. No acceptance/raw relabel.'))
(pair/'hash-chain-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(controls=len(rejected),seconds=result['seconds'])))
