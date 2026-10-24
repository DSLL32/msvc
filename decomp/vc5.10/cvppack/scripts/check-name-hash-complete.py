#!/usr/bin/env python3
"""Affected stream ABI, initialized-write and ended-temporary checks; native only."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0109-fill-name-stream'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=module('complete_raw','raw-state.py');initial=module('complete_initial','name-hash-state.py');complete=module('complete_hash','name-hash-complete-state.py');public=module('complete_public','public-writer-state.py');source=module('complete_source','source-allocation-roles.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((pair/'native/state.json').read_text())
def compare(a,b):source.source_family_indices(a,b);owners=initial.generations(a,b);public.validate(config,data,a,b);return complete.validate(config,data,a,b,owners,initial)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason,changed_raw=None):
 a=copy.deepcopy(original) if changed_raw is None else raw.decode(changed_raw);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-real-addresshash-frontier',lambda a,b:a.update(address=0x410010),'addresshash child')
reject('missing-initial-observation',lambda a,b:a['name_hash_first_fill_stack'].update(bytes=b''),'first-fill stack observation')
reject('wrong-first-fill-actual-stack',lambda a,b:a['name_hash_first_fill_stack'].update(address=a['name_hash_first_fill_stack']['address']+4),'entry stack')
reject('wrong-offset-local-type',lambda a,b:b['name_hash_frame'].update(offset_type='unsigned short'),'DWORD offset local')
reject('wrong-offset-local-owner',lambda a,b:b['name_hash_frame'].update(offset_address=b['name_hash_frame']['offset_address']+4),'copy pointer/owner')
reject('missing-stream-call',lambda a,b:b['name_hash_stream_calls'].pop(),'stream inventory')
reject('extra-stream-call',lambda a,b:b['name_hash_stream_calls'].append(copy.deepcopy(b['name_hash_stream_calls'][-1])),'stream inventory')
reject('wrong-original-copy-site',lambda a,b:a['name_hash_stream_calls'][2].update(site=0),'stream sequence/owner')
reject('wrong-native-call-owner',lambda a,b:b['name_hash_stream_calls'][2].update(owner='write_publics'),'stream sequence/owner')
reject('wrong-copy-width',lambda a,b:b['name_hash_stream_calls'][2].update(size=2),'stream size/arena')
reject('wrong-copy-arena',lambda a,b:b['name_hash_stream_calls'][2].update(arena=b['public_writer_frame']['stream_objects'][1]['address']),'stream size/arena')
reject('wrong-original-fastcall-cleanup',lambda a,b:a['name_hash_stream_calls'][2].update(exit_esp=0),'fastcall/cdecl cleanup')
reject('wrong-native-fill-cleanup',lambda a,b:b['name_hash_stream_calls'][1].update(exit_esp=0),'fastcall/cdecl cleanup')
reject('wrong-cdecl-fill-formal',lambda a,b:b['name_hash_stream_calls'][1]['formal_types'].update(size='unsigned short'),'formal ABI')
reject('clobbered-fill-EBP',lambda a,b:b['name_hash_stream_calls'][1]['after_nonvolatile'].__setitem__(3,0),'nonvolatile preservation')
reject('wrong-offset-copy-input',lambda a,b:b['name_hash_stream_calls'][3].update(input='00'*4),'initialized copy input')
reject('wrong-count-copy-pointer',lambda a,b:a['name_hash_stream_calls'][8].update(value=0),'copy pointer/owner')
reject('wrong-fill-byte',lambda a,b:b['name_hash_stream_calls'][3].update(kind=1),'stream sequence/owner')
reject('wrong-actual-zero-fill-value',lambda a,b:b['name_hash_stream_calls'][9].update(value=1),'fill byte')
reject('wrong-copy-input-arena',lambda a,b:b['name_hash_stream_calls'][3].update(before_arena='00'*24),'input state/result')
reject('wrong-fill-return-pointer',lambda a,b:b['name_hash_stream_calls'][1].update(result=0),'input state/result')
reject('wrong-fill-after-arena',lambda a,b:b['name_hash_stream_calls'][1].update(after_arena='00'*24),'initialized stream writes')
reject('wrong-copy-output-prefix',lambda a,b:b['name_hash_stream_calls'][3].update(output='00'*12),'initialized stream writes')
reject('missing-free',lambda a,b:b['name_hash_temp_frees'].pop(),'temporary free inventory')
reject('wrong-free-site',lambda a,b:a['name_hash_temp_frees'][0].update(site=0),'free sequence')
reject('wrong-free-generation',lambda a,b:b['name_hash_temp_frees'][0].update(address=b['name_hash_allocations'][0]['address']),'free generation/owner')
reject('wrong-canonical-free-provider',lambda a,b:a['name_hash_temp_frees'][0].update(provider=0),'canonical free provider/return')
reject('unreturned-free',lambda a,b:b['name_hash_temp_frees'][0].update(returned=False),'canonical free provider/return')
reject('wrong-free-stack',lambda a,b:b['name_hash_temp_frees'][0].update(exit_esp=0),'free cdecl stack')
reject('clobbered-free-ESI',lambda a,b:b['name_hash_temp_frees'][0]['after_nonvolatile'].__setitem__(1,0),'free normal-return registers')
reject('truncated-prefree-range',lambda a,b:b['name_hash_temp_frees'][0].update(size=623),'prefree initialized extent')
reject('changed-prefree-cleared-tail',lambda a,b:b['name_hash_temp_frees'][2].update(before=b['name_hash_temp_frees'][2]['before'][:-2]+'ff'),'prefree initialized contents')
reject('NULL-free-payload',lambda a,b:b['name_hash_temp_frees'][3].update(before='00'),'NULL free admitted payload')
reject('wrong-current-live-arena',lambda a,b:b['name_hash_complete'].update(arena='00'*24),'live arena identity/fields')
reject('wrong-current-live-block-next',lambda a,b:b['name_hash_complete'].update(block=b['name_hash_complete']['block'][:16]+'01000000'),'complete live output')
reject('changed-current-live-output',lambda a,b:b['name_hash_complete'].update(payload='ff'+b['name_hash_complete']['payload'][2:]),'complete live output')
reject('claimed-own-unwritten-tail',lambda a,b:b['name_hash_complete'].update(payload=b['name_hash_complete']['payload']+'00'*3532),'complete live output')
def cross(a,b):
 base=b['name_hash_allocations'][0]['address'];b['memory'][str(base-4)]='00'*8
reject('retired-before-base-crossing-current-memory',cross,'leaked into generic current memory')
def string_cross(a,b):
 base=a['name_hash_allocations'][1]['address'];a['strings'][base-1]=b'\0\0'
reject('retired-before-base-crossing-current-string',string_cross,'leaked into generic current memory')
line=next(line for line in events.splitlines() if line.startswith('namehash-stream-input ') and line.split()[2]=='4');f=line.split();f[-1]='01000000'
reject('raw-mutated-offset-input',lambda a,b:None,'initialized copy input',events.replace(line,' '.join(f),1))
a=copy.deepcopy(original);payload=bytearray(a['memory'][a['esp']]);payload[84:108]=bytes(range(24));a['memory'][a['esp']]=bytes(payload);snapshot=bytearray(a['name_hash_first_fill_stack']['bytes']);snapshot[200:224]=bytes(reversed(range(24)));a['name_hash_first_fill_stack']['bytes']=bytes(snapshot);assert compare(a,native)==baseline
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,positive_own_uninitialized_second_stream_bits=True,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/name-hash-complete-state.py'),initial_validator_sha256=sha(BINARY/'scripts/name-hash-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0108-name-hash/name-hash-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0108-name-hash/name-hash-checks.json'),scope='Prior57 source/generation/table/reused-init ABI controls retain their historical scopes. Initial namehash controller unchanged. New controls check actual first-fill historical projection, all28 stream calls, input/output/ABI,12 frees and current versus prefree namespaces. Full production positive separately checks all inherited state and actual375 sort callbacks.'))
(pair/'complete-name-hash-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
