#!/usr/bin/env python3
"""Focused ownership, generation, field, initialized-byte and reused-ABI checks."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0108-name-hash';native_dir=pair/'native-layout-corrected'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=module('hash_raw','raw-state.py');hash_state=module('hash_state','name-hash-state.py');public=module('hash_public','public-writer-state.py');source=module('hash_source','source-allocation-roles.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((native_dir/'state.json').read_text())
def compare(a,b):source.source_family_indices(a,b);owners=hash_state.generations(a,b);public.validate(config,data,a,b);return hash_state.validate(config,data,a,b,owners)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason,changed_raw=None):
 a=copy.deepcopy(original) if changed_raw is None else raw.decode(changed_raw);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
source_indices=sorted(source.source_family_indices(original,native))
reject('source-wrong-owner',lambda a,b:b['checked_allocations'][source_indices[0]].update(owner='unknown'),'three consecutive owners')
def source_generation(a,b):
 b['checked_allocations'][source_indices[0]]['after_frees']=11;b['source_family_allocations'][0]['after_frees']=11
reject('source-wrong-cleanup-generation',source_generation,'cleanup generation')
reject('source-wrong-original-generation',lambda a,b:a['source_family_allocations'][0].update(address=a['source_family_allocations'][0]['address']+4),'original generation')
def nonconsecutive(a,b):
 x,y=source_indices[0]-1,source_indices[1]
 for state in (a,b):state['checked_allocations'][x],state['checked_allocations'][y]=state['checked_allocations'][y],state['checked_allocations'][x]
reject('source-nonconsecutive-events',nonconsecutive,'three consecutive owners')
def extra_source(a,b):
 for state in (a,b):state['checked_allocations'].append(copy.deepcopy(state['checked_allocations'][source_indices[0]]))
reject('source-extra-event',extra_source,'three consecutive owners')
reject('missing-new-owner',lambda a,b:b['name_hash_allocations'].pop(),'new generation inventory')
reject('wrong-new-owner',lambda a,b:b['name_hash_allocations'][3].update(owner='build_symbol_name_hash'),'allocation owner/ABI')
reject('wrong-new-context',lambda a,b:b['name_hash_allocations'][0].update(context='unknown'),'allocation owner/ABI')
reject('wrong-new-allocation-kind',lambda a,b:b['name_hash_allocations'][0].update(kind='cleared'),'allocation owner/ABI')
reject('wrong-actual-allocation-site',lambda a,b:a['name_hash_allocations'][0].update(site=0),'allocator call/size/result')
reject('wrong-actual-return-site',lambda a,b:a['name_hash_allocations'][0].update(return_site=0),'allocator call/size/result')
reject('wrong-actual-request-size',lambda a,b:a['name_hash_allocations'][1].update(size=623),'allocator call/size/result')
reject('wrong-original-allocator-stack',lambda a,b:a['name_hash_allocations'][0].update(exit_esp=0),'allocator cdecl return stack')
reject('wrong-native-allocator-stack',lambda a,b:b['name_hash_allocations'][0].update(exit_esp=0),'allocator cdecl return stack')
reject('wrong-actual-allocation-generation',lambda a,b:b['name_hash_allocations'][0].update(after_frees=11),'generation ordering')
reject('clobbered-allocator-ESI',lambda a,b:b['name_hash_allocations'][0]['after_nonvolatile'].__setitem__(1,b['name_hash_allocations'][0]['after_nonvolatile'][1]^1),'allocator return registers')
new_indices=[i for i,r in enumerate(native['checked_allocations']) if r.get('context')=='build_symbol_name_hash']
reject('wrong-generic-private-event',lambda a,b:b['checked_allocations'][new_indices[0]].update(address=b['checked_allocations'][new_indices[0]]['address']+4),'generation/event identity')
def change_address(b,index,address):
 row=b['name_hash_allocations'][index];kind=row['kind'];rows=b[kind+'_allocations'];matches=[r for r in rows if r.get('context')=='build_symbol_name_hash' and r['address']==row['address'] and r['size']==row['size']];assert len(matches)==1;matches[0]['address']=address;row['address']=address
reject('coherent-partially-overlapping-new-owners',lambda a,b:change_address(b,1,b['name_hash_allocations'][0]['address']+2),'new allocations overlap')
reject('coherent-live-directory-spare-overlap',lambda a,b:change_address(b,0,b['directory_allocation']['address']+840),'overlaps live directory')
reject('coherent-live-output-unwritten-tail-overlap',lambda a,b:change_address(b,0,b['output_buffer_allocations'][0]['address']+65508),'overlaps live packed-output')
reject('coherent-live-mapping-tail-overlap',lambda a,b:change_address(b,0,int(next(iter(b['mapped_views'])))+2097148),'overlaps live mapped-input')
reject('wrong-six-formal-type',lambda a,b:b['name_hash_frame']['formal_types'].update(method='unsigned int *'),'six formal types')
reject('missing-six-formal-type',lambda a,b:b['name_hash_frame']['formal_types'].pop('buckets'),'six formal types')
reject('wrong-named-parent',lambda a,b:b['name_hash_frame'].update(caller='main'),'local owner')
reject('wrong-original-incoming-callback',lambda a,b:a['name_hash_entry']['stack'].__setitem__(1,0),'six incoming original arguments')
reject('wrong-original-incoming-WORD-out',lambda a,b:a['name_hash_entry']['stack'].__setitem__(2,a['name_hash_entry']['stack'][2]+2),'six incoming original arguments')
reject('wrong-native-incoming-count',lambda a,b:b['name_hash_entry']['stack'].__setitem__(5,63),'incoming/formal roles')
reject('wrong-current-bucket-layout',lambda a,b:b['name_hash_frame']['layouts']['NameHashBucket']['fields']['overflow'].update(offset=8),'bucket/overflow layouts')
reject('wrong-current-overflow-layout',lambda a,b:b['name_hash_frame']['layouts']['NameHashOverflow'].update(width=48),'bucket/overflow layouts')
reject('wrong-current-bucket-count',lambda a,b:b['name_hash_frame']['scalars'].update(hash_count=8),'initialized scalar state')
reject('wrong-current-overflow-page-alias',lambda a,b:b['name_hash_frame'].update(overflow_pages='00'*40),'overflow-page pointer array')
def owned_change(b,index,offset):
 key=str(b['name_hash_allocations'][index]['address']);payload=bytearray.fromhex(b['name_hash_frame']['initialized_allocations'][key]);payload[offset]^=1;b['name_hash_frame']['initialized_allocations'][key]=payload.hex()
reject('wrong-initialized-page-table-pointer',lambda a,b:owned_change(b,0,0),'page-table pointer')
reject('changed-initialized-bucket-count',lambda a,b:owned_change(b,1,0),'initialized bucket contents')
reject('changed-cleared-bucket-reserved-WORD',lambda a,b:owned_change(b,1,2),'initialized bucket contents')
reject('changed-bucket-overflow-alias',lambda a,b:owned_change(b,1,3*104+4),'initialized bucket contents')
reject('changed-cleared-inline-spare',lambda a,b:owned_change(b,1,72),'initialized bucket contents')
reject('changed-overflow-entry',lambda a,b:owned_change(b,2,4),'cleared overflow contents')
reject('changed-cleared-overflow-spare',lambda a,b:owned_change(b,2,100),'cleared overflow contents')
reject('changed-initialized-output-block',lambda a,b:owned_change(b,3,0),'output block')
reject('changed-output-next-pointer',lambda a,b:owned_change(b,3,8),'output block')
reject('changed-initialized-payload',lambda a,b:owned_change(b,4,0),'output payload')
def expanded_payload(a,b):
 key=str(b['name_hash_allocations'][4]['address']);b['name_hash_frame']['initialized_allocations'][key]+='00'*4094
reject('claimed-unwritten-payload-as-initialized',expanded_payload,'output payload')
reject('missing-contextual-init',lambda a,b:b['name_hash_reused_calls'].pop('initialize_arena'),'reused arena-call inventory')
reject('wrong-reused-init-capacity',lambda a,b:b['name_hash_reused_calls']['initialize_arena'].update(capacity=8192),'arena initialization')
reject('wrong-reused-copy-fastcall-cleanup',lambda a,b:b['name_hash_reused_calls']['copy_type_arena'].update(exit_esp=b['name_hash_reused_calls']['copy_type_arena']['exit_esp']-4),'cdecl/fastcall cleanup')
reject('wrong-reused-copy-actual-type',lambda a,b:b['name_hash_reused_calls']['copy_type_arena']['formal_types'].update(size='unsigned short'),'reused actual formal inventory')
reject('changed-reused-copy-input',lambda a,b:b['name_hash_reused_calls']['copy_type_arena'].update(input='0700'),'copied WORD input/owner')
reject('wrong-reused-copy-WORD-owner',lambda a,b:b['name_hash_reused_calls']['copy_type_arena'].update(data=b['name_hash_reused_calls']['copy_type_arena']['data']+2),'copied WORD input/owner')
reject('wrong-reused-copy-return',lambda a,b:b['name_hash_reused_calls']['copy_type_arena'].update(result=0),'copy input/result')
reject('changed-current-defined-arena',lambda a,b:b['name_hash_frame'].update(arena_bytes='00'*24),'fully initialized arena fields')
reject('wrong-remaining-uninitialized-stream-count',lambda a,b:b['public_writer_frame'].update(uninitialized_stream_bytes_uncompared=48),'unknown stream state')
reject('wrong-fill-value',lambda a,b:b['parameters'].update(value=1),'output pointer/scalar ABI')
reject('wrong-fill-size',lambda a,b:b['parameters'].update(size=4),'output pointer/scalar ABI')
reject('wrong-fill-output-role',lambda a,b:b['parameters']['arena'].update(local='addresses'),'output pointer/scalar ABI')
def leaked(a,b):
 row=b['name_hash_allocations'][4];b['memory'][str(row['address']+4092)]='00'*4
reject('new-owner-tail-leaked-into-generic-memory',leaked,'leaked into generic current memory')
def crossing(a,b):
 address=b['name_hash_allocations'][0]['address'];b['memory'][str(address-4)]='00'*8
reject('new-owner-before-base-crossing-generic-memory',crossing,'leaked into generic current memory')
line=next(line for line in events.splitlines() if line.startswith('namehash-owned-bytes ') and line.split()[2]=='624');f=line.split();f[-1]='ff'+f[-1][2:]
reject('raw-changed-current-bucket',lambda a,b:None,'initialized bucket contents',events.replace(line,' '.join(f),1))
a=copy.deepcopy(original);contents=bytearray(a['memory'][a['esp']]);contents[200:224]=bytes(range(24));a['memory'][a['esp']]=bytes(contents);assert compare(a,native)==baseline
result=dict(schema=1,native_only=True,scope='Production source-generation, new-allocation, public-local and namehash state validators over actual decoded pair. Full production positive separately checks all previous typed-state/ABI/graph/import/lifetime rules. Mutants refresh coherent typed relationships where needed so intended generation/overlap checks are exercised, not merely stale raw hashes.',fresh_negative_count=len(rejected),rejected=rejected,positive_own_uninitialized_second_stream_bits=True,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/name-hash-state.py'),source_roles_sha256=sha(BINARY/'scripts/source-allocation-roles.py'),public_validator_sha256=sha(BINARY/'scripts/public-writer-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(native_dir/'receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0107-public-page-padding/padding-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0107-public-page-padding/padding-checks.json'),scope='Prior33 padding/source-node controls and earlier32public/24copy/24allocation/32sort controls retain their historical scopes. New controls here exercise changed source event classification, five new generations/current contents, reused contextual ABI and only24B remaining unknown stream. Current fresh prefix repeats actual previous behavior.'))
(pair/'name-hash-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
