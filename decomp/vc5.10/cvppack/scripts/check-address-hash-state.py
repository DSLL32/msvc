#!/usr/bin/env python3
"""Affected actual event bounds, current owners, sort inputs and callback checks."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0110-address-hash-first-sort'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=module('address_raw','raw-state.py');initial=module('address_initial','name-hash-state.py');address=module('address_state','address-hash-state.py');public=module('address_public','public-writer-state.py');source=module('address_source','source-allocation-roles.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((pair/'native/state.json').read_text())
def compare(a,b):source.source_family_indices(a,b);owners=address.generations(a,b);initial.generations(a,b);public.validate(config,data,a,b);return address.validate(config,data,a,b,owners)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason,changed_raw=None):
 a=copy.deepcopy(original) if changed_raw is None else raw.decode(changed_raw);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('missing-current-owner',lambda a,b:b['address_hash_allocations'].pop(),'allocation inventory')
reject('wrong-actual-entry-checked-bound',lambda a,b:b['address_hash_entry'].update(checked_count=0),'checked-event entry bound')
reject('wrong-actual-entry-cleared-bound',lambda a,b:b['address_hash_entry'].update(cleared_count=0),'cleared-event entry bound')
def coherent_bound(a,b):
 a['address_hash_entry']['cleared_count']+=1;b['address_hash_entry']['cleared_count']+=1
reject('coherently-forged-entry-bound',coherent_bound,'cleared-event entry bound')
reject('wrong-current-owner',lambda a,b:b['address_hash_allocations'][0].update(owner='build_symbol_name_hash'),'owner/generation/ABI')
reject('wrong-current-generation',lambda a,b:b['address_hash_allocations'][0].update(after_namehash_frees=11),'owner/generation/ABI')
reject('wrong-current-site',lambda a,b:a['address_hash_allocations'][0].update(site=0),'allocation site/size/result')
reject('wrong-current-return-site',lambda a,b:a['address_hash_allocations'][0].update(return_site=0),'allocation site/size/result')
reject('wrong-current-request',lambda a,b:b['address_hash_allocations'][1].update(size=208),'allocation site/size/result')
reject('wrong-current-cleanup',lambda a,b:b['address_hash_allocations'][0].update(exit_esp=0),'allocator cdecl cleanup')
reject('clobbered-allocator-EDI',lambda a,b:b['address_hash_allocations'][0]['after_nonvolatile'].__setitem__(2,0),'normal-return registers')
new_start=native['address_hash_entry']['cleared_count']
reject('wrong-generic-event-context',lambda a,b:b['cleared_allocations'][new_start].update(context='unknown'),'allocation event identity')
reject('wrong-generic-event-address',lambda a,b:b['cleared_allocations'][new_start].update(address=b['cleared_allocations'][new_start]['address']+4),'allocation event identity')
def relocate(b,index,where):
 b['address_hash_allocations'][index]['address']=where;b['cleared_allocations'][new_start+index]['address']=where
reject('coherent-partial-current-owner-overlap',lambda a,b:relocate(b,0,b['address_hash_allocations'][1]['address']-4),'current allocations overlap')
reject('coherent-live-name-payload-tail-overlap',lambda a,b:relocate(b,0,b['name_hash_allocations'][4]['address']+4092),'overlaps live prior generation')
reject('coherent-live-name-block-overlap',lambda a,b:relocate(b,0,b['name_hash_allocations'][3]['address']),'overlaps live prior generation')
# Later live events may reuse an ended range, but may not overlap a new
# current owner. These three were misclassified by address containment.
for live_kind,live_index in [('module',0),('image',4),('checked',100),('cleared',15)]:
 def later_live(a,b,kind=live_kind,index=live_index):
  live=b[kind+'_allocations'][index];live['address']=b['address_hash_allocations'][0]['address'];live['size']=8
 reject('live-'+live_kind+'-generation-inside-older-ended-range',later_live,'overlaps live prior generation')
reject('wrong-ended-allocation-generation',lambda a,b:b['cleared_allocations'][8].update(address=b['cleared_allocations'][8]['address']+4),'ended allocation event/free identity')
reject('wrong-ended-source-generation-role',lambda a,b:b['checked_allocations'][295].update(role='output'),'source family actual checked owner/role')
reject('coherent-directory-spare-overlap',lambda a,b:relocate(b,0,b['directory_allocation']['address']+840),'overlaps live directory')
reject('coherent-output-tail-overlap',lambda a,b:relocate(b,0,b['output_buffer_allocations'][0]['address']+65508),'overlaps live packed-output')
reject('coherent-mapping-tail-overlap',lambda a,b:relocate(b,0,int(next(iter(b['mapped_views'])))+2097148),'overlaps live mapped-input')
reject('wrong-original-incoming-WORD-output',lambda a,b:a['address_hash_entry']['stack'].__setitem__(1,0),'original entry stack')
reject('wrong-native-incoming-output-owner',lambda a,b:b['address_hash_entry']['stack'].__setitem__(1,b['address_hash_entry']['stack'][1]-2),'native five incoming roles')
reject('wrong-native-incoming-count',lambda a,b:b['address_hash_entry']['stack'].__setitem__(4,63),'native five incoming roles')
reject('wrong-five-formal-type',lambda a,b:b['address_hash_entry']['formal_types'].update(method='unsigned int *'),'formal types/lifetime')
reject('wrong-after-free-entry-phase',lambda a,b:b['address_hash_entry'].update(after_namehash_frees=11),'formal types/lifetime')
reject('wrong-qsort-provider',lambda a,b:a['address_hash_sort'].update(provider=0),'canonical qsort caller/ABI')
reject('wrong-original-qsort-stack',lambda a,b:a['address_hash_sort'].update(entry_esp=0),'canonical qsort caller/ABI')
reject('wrong-qsort-width',lambda a,b:b['address_hash_sort']['args'].__setitem__(2,12),'qsort arguments/owner')
reject('wrong-qsort-count',lambda a,b:b['address_hash_sort']['args'].__setitem__(1,26),'qsort arguments/owner')
reject('wrong-qsort-callback',lambda a,b:b['address_hash_sort']['args'].__setitem__(3,b['function_addresses']['compare_packed_directory_entries']),'qsort arguments/owner')
reject('wrong-qsort-input-owner',lambda a,b:b['address_hash_sort']['args'].__setitem__(0,b['address_hash_allocations'][2]['address']),'qsort arguments/owner')
reject('wrong-maximum-segment',lambda a,b:b['address_hash_sort']['scalars'].update(maximum_segment=4),'local scalars/group pointer')
reject('wrong-absolute-count',lambda a,b:b['address_hash_sort']['scalars'].update(absolute_count=1),'local scalars/group pointer')
reject('wrong-entry-layout',lambda a,b:b['address_hash_sort']['layouts']['AddressHashEntry']['fields']['stream_offset'].update(offset=0),'field layout')
reject('wrong-group-width',lambda a,b:b['address_hash_sort']['layouts']['AddressHashGroup'].update(width=16),'field layout')
def owned_change(b,phase,index,offset):
 key=str(b['address_hash_allocations'][index]['address']);table=b['address_hash_sort']['before_owned'] if phase=='before' else b['address_hash_current_owned'];contents=bytearray.fromhex(table[key]);contents[offset]^=1;table[key]=contents.hex()
reject('changed-before-group-count',lambda a,b:owned_change(b,'before',0,0),'group counts/aliases')
reject('changed-current-group-pointer',lambda a,b:owned_change(b,'current',0,8),'group counts/aliases')
reject('changed-before-input-record-owner',lambda a,b:owned_change(b,'before',1,0),'before-sort initialized rows')
reject('changed-before-input-stream-offset',lambda a,b:owned_change(b,'before',1,4),'before-sort initialized rows')
reject('wrong-first-pivot-row',lambda a,b:owned_change(b,'current',1,0),'callback array mutation')
reject('changed-unvisited-second-group',lambda a,b:owned_change(b,'current',2,4),'callback array mutation')
reject('truncated-current-third-group',lambda a,b:b['address_hash_current_owned'].update({str(b['address_hash_allocations'][3]['address']):'00'*56}),'callback array mutation')
reject('wrong-two-formal-pointer-type',lambda a,b:b['observed_argument_types'][0].update(native_type='unsigned int *'),'two-formal ABI')
reject('missing-two-formal-inventory',lambda a,b:b['observed_argument_types'].pop(),'two-formal ABI')
reject('wrong-first-row-alias',lambda a,b:b['parameters']['first'].update(index=0),'comparator argument aliases')
def crossing(a,b):
 base=b['address_hash_allocations'][0]['address'];b['memory'][str(base-4)]='00'*8
reject('current-owner-before-base-crossing-memory',crossing,'leaked into generic current namespace')
def stringcross(a,b):
 base=a['address_hash_allocations'][1]['address'];a['strings'][base-1]=b'\0\0'
reject('current-owner-before-base-crossing-string',stringcross,'leaked into generic current namespace')
line=next(line for line in events.splitlines() if line.startswith('addresshash-owned before-sort ') and line.split()[3]=='216');f=line.split();f[-1]='ff'+f[-1][2:]
reject('raw-mutated-complete-before-array',lambda a,b:None,'before-sort initialized rows',events.replace(line,' '.join(f),1))
a=copy.deepcopy(original);entry=bytearray(a['address_hash_entry']['parent_stack']);entry[84:108]=bytes(range(24));a['address_hash_entry']['parent_stack']=bytes(entry);assert compare(a,native)==baseline
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,positive_own_uninitialized_address_stream_bits=True,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/address-hash-state.py'),historical_generation_validator_sha256=sha(BINARY/'scripts/name-hash-state.py'),public_validator_sha256=sha(BINARY/'scripts/public-writer-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused=[dict(path='cases/hello-debug-dll/evidence/0109-fill-name-stream/complete-name-hash-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0109-fill-name-stream/complete-name-hash-checks.json'),scope='Prior40 stream/free controls retain historical scope. Current full positive freshly checks all28stream/12free proofs, while new controls cover actual later event bounds, current owners, qsort first-call group mutation and callback ABI. Legacy generation/stream predicates are retained; new addresshash-entry branch uses actual event indices rather than owner filtering.')])
(pair/'address-hash-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
