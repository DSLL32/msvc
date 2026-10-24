#!/usr/bin/env python3
"""Affected complete address-sort/stream/lifetime controls, native only."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0111-address-comparator-output'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
raw=module('complete_address_raw','raw-state.py');controller=module('complete_address_state','address-hash-complete-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text());artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/folder).glob(pattern)} for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native','native-view-*.bin')]}
full=raw.compare(config,data,events,native,artifacts,views)
phase_events,previous,phase=controller.first_phase(events,native);historical=raw.decode(phase_events);lines=events.splitlines();begin=lines.index('addresshash-phase-entry 0040edc0');end=lines.index('addresshash-phase-end 0040edc0');original=raw.decode('\n'.join(lines[:begin]+[line for line in lines[begin:end+1] if line.startswith('mapped-view ')]+lines[end+1:])+'\n');observations=controller.decode(events);policy=controller.prove(config);phase=full['fresh_historical_phase']
def compare(a,b,c):return controller.validate_current(config,data,a,b,historical,b['address_hash_first_callback'],c,b['address_hash_complete'],full['prefix'],phase,policy)
assert compare(original,native,observations)==full
rejected=[]
def reject(name,mutate,reason):
 a=copy.deepcopy(original);b=copy.deepcopy(native);c=copy.deepcopy(observations);mutate(a,b,c)
 try:compare(a,b,c)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
def change_hex(row,key,offset=0):
 payload=bytearray.fromhex(row[key]);payload[offset]^=1;row[key]=payload.hex()
reject('wrong-final-named-frontier',lambda a,b,c:a.update(address=0x40edc0),'real next frontier')
reject('missing-complete-sort',lambda a,b,c:b['address_hash_complete']['sorts'].pop(),'sort inventory')
reject('wrong-sort-width',lambda a,b,c:b['address_hash_complete']['sorts'][1]['args'].__setitem__(2,12),'sort arguments')
reject('wrong-sort-provider',lambda a,b,c:c['sorts'][0].update(provider=0),'provider/owner')
reject('wrong-sort-stack-cleanup',lambda a,b,c:b['address_hash_complete']['sorts'][0].update(exit_esp=0),'qsort cdecl stack')
reject('wrong-sort-callback-count',lambda a,b,c:b['address_hash_complete']['sorts'][0].update(callback_end=0),'callback group bounds')
def array_change(b,sort,phase,index,offset=0):
 root=str(b['address_hash_allocations'][index]['address']);row=b['address_hash_complete']['sorts'][sort][phase+'_owned'];payload=bytearray.fromhex(row[root]);payload[offset]^=1;row[root]=payload.hex()
reject('changed-before-row-offset',lambda a,b,c:array_change(b,0,'before',1,4),'before-sort arrays')
reject('changed-postsort-row-offset',lambda a,b,c:array_change(b,0,'after',1,4),'post-sort arrays')
reject('changed-other-owner-group-count',lambda a,b,c:array_change(b,0,'after',0,0),'post-sort arrays')
reject('missing-callback',lambda a,b,c:b['address_hash_complete']['callbacks'].pop(),'sort inventory')
reject('wrong-callback-sequence',lambda a,b,c:b['address_hash_complete']['callbacks'][0].update(sequence=1),'callback sequence')
reject('wrong-callback-group',lambda a,b,c:b['address_hash_complete']['callbacks'][0].update(sort=1),'callback sequence')
reject('wrong-canonical-return-role',lambda a,b,c:c['callbacks'][0].update(return_pc=0),'return instruction')
reject('wrong-native-return-instruction',lambda a,b,c:b['address_hash_complete']['callbacks'][0].update(return_pc=0),'return instruction')
reject('wrong-callback-row-pointer',lambda a,b,c:b['address_hash_complete']['callbacks'][0].update(first=0),'row-pointer alias')
reject('changed-callback-input',lambda a,b,c:change_hex(b['address_hash_complete']['callbacks'][0],'records',4),'row contents')
reject('wrong-comparator-result',lambda a,b,c:b['address_hash_complete']['callbacks'][0].update(result=123456),'signed subtraction')
reject('wrong-callback-cleanup',lambda a,b,c:b['address_hash_complete']['callbacks'][0].update(exit_esp=0),'cdecl stack/return')
reject('wrong-real-formal-pointer-type',lambda a,b,c:b['address_hash_complete']['callbacks'][0]['formal_inventory'].update(first='unsigned int *'),'two-formal types')
reject('changed-callback-own-FP-payload',lambda a,b,c:b['address_hash_complete']['callbacks'][0]['after']['fp'].update(st7='0x'+'ff'*10),'own raw FP preservation')
reject('changed-callback-own-errno',lambda a,b,c:b['address_hash_complete']['callbacks'][0]['after'].update(errno=77),'own error identity/state')
reject('changed-sort-own-LastError-storage',lambda a,b,c:b['address_hash_complete']['sorts'][1]['after'].update(last_error_address=0),'own error identity/state')
reject('changed-callback-EDI',lambda a,b,c:b['address_hash_complete']['callbacks'][0]['after']['nonvolatile'].__setitem__(2,0),'normal-return registers')
reject('missing-new-output-owner',lambda a,b,c:b['address_hash_complete']['allocations'].pop(),'output allocation inventory')
reject('wrong-new-output-size',lambda a,b,c:b['address_hash_complete']['allocations'][1].update(size=4088),'allocation site/size')
reject('wrong-new-output-owner',lambda a,b,c:b['address_hash_complete']['allocations'][0].update(context='build_symbol_name_hash'),'allocation owner/ABI')
reject('wrong-new-output-allocation-cleanup',lambda a,b,c:b['address_hash_complete']['allocations'][0].update(exit_esp=0),'allocation return ABI')
reject('wrong-new-output-event-generation',lambda a,b,c:b['cleared_allocations'][114].update(address=0),'generic-event identity')
def relocate_output(b,index,address):
 b['address_hash_complete']['allocations'][index]['address']=address;b['cleared_allocations' if index==0 else 'checked_allocations'][114 if index==0 else 300]['address']=address
reject('coherent-new-output-overlaps-live-name-tail',lambda a,b,c:relocate_output(b,0,b['name_hash_allocations'][4]['address']+4092),'earlier live generation')
reject('coherent-new-output-overlaps-live-temp-array',lambda a,b,c:relocate_output(b,0,b['address_hash_allocations'][1]['address']),'earlier live generation')
reject('forged-carried-forward-allocation-event',lambda a,b,c:b['checked_allocations'][100].update(size=1),'carried allocation-event prefix')
reject('forked-full-versus-phase-array-owner',lambda a,b,c:b['address_hash_allocations'][0].update(address=0),'carried lifecycle/resource prefix')
reject('unknown-late-file-call',lambda a,b,c:b['file_calls'].append(dict(unknown=True)),'carried lifecycle/resource prefix')
reject('changed-current-native-view-catalog',lambda a,b,c:next(iter(b['mapped_views'].values())).update(length=1),'mapped-view lifetime catalog')
reject('wrong-actual-arena-capacity',lambda a,b,c:b['address_hash_complete']['initialization'].update(capacity=8192),'arena initialization')
reject('wrong-actual-init-formal',lambda a,b,c:b['address_hash_complete']['initialization']['formal_types'].update(capacity='unsigned short'),'initializer cdecl ABI')
reject('missing-stream-operation',lambda a,b,c:b['address_hash_complete']['stream_calls'].pop(),'operation inventory')
reject('wrong-stream-owner',lambda a,b,c:b['address_hash_complete']['stream_calls'][0].update(owner='unknown'),'sequence/owner')
reject('wrong-stream-copy-size',lambda a,b,c:b['address_hash_complete']['stream_calls'][0].update(size=4),'extent/arena')
reject('wrong-stream-caller-arena',lambda a,b,c:b['address_hash_complete']['stream_calls'][0].update(arena=0),'extent/arena')
reject('wrong-copy-fastcall-cleanup',lambda a,b,c:b['address_hash_complete']['stream_calls'][0].update(exit_esp=0),'fastcall/cdecl cleanup')
reject('wrong-DWORD-offset-source-type',lambda a,b,c:b['address_hash_complete']['stream_calls'][2]['scalar_sources']['offset'].update(width=2),'scalar-local ABI')
reject('wrong-actual-scalar-source-pointer',lambda a,b,c:b['address_hash_complete']['stream_calls'][0].update(value=0),'copy source/bytes')
reject('changed-complete-copy-input',lambda a,b,c:change_hex(b['address_hash_complete']['stream_calls'][0],'input'),'copy source/bytes')
reject('wrong-fill-byte',lambda a,b,c:b['address_hash_complete']['stream_calls'][1].update(value=1),'fill byte')
reject('changed-stream-arena-result',lambda a,b,c:b['address_hash_complete']['stream_calls'][1].update(result=0),'arena/result identity')
reject('changed-stream-prefix-write',lambda a,b,c:change_hex(b['address_hash_complete']['stream_calls'][-1],'output',539),'initialized stream write')
reject('missing-real-free',lambda a,b,c:b['address_hash_complete']['frees'].pop(),'free inventory')
reject('wrong-free-generation',lambda a,b,c:b['address_hash_complete']['frees'][0].update(index=2),'free sequence/owner')
reject('wrong-free-pointer',lambda a,b,c:b['address_hash_complete']['frees'][0].update(address=0),'free sequence/owner')
reject('changed-prefree-sorted-array',lambda a,b,c:change_hex(b['address_hash_complete']['frees'][0],'before',4),'pre-free initialized bytes')
reject('wrong-free-provider',lambda a,b,c:c['frees'][0].update(provider=0),'canonical provider/return')
reject('wrong-free-stack',lambda a,b,c:b['address_hash_complete']['frees'][0].update(exit_esp=0),'free cdecl/nonvolatile')
reject('wrong-current-output-block-alias',lambda a,b,c:b['address_hash_complete']['current'].update(block_address=0),'initialized block/output')
reject('changed-current-output-byte',lambda a,b,c:change_hex(b['address_hash_complete']['current'],'payload',539),'initialized block/output')
reject('wrong-current-names-argument',lambda a,b,c:b['parameters']['stream'].update(local='addresses'),'next actual names formal/alias')
reject('wrong-current-names-formal-width',lambda a,b,c:b['observed_argument_types'][0].update(pointee_width=4),'next actual names formal/alias')
reject('changed-current-live-names-output',lambda a,b,c:change_hex(b['name_hash_complete'],'payload',0),'current live names generation')
def crossing(b,namespace,index):
 address=b['address_hash_allocations'][index]['address'];b[namespace][str(address-4)]='00'*8
reject('retired-array-before-base-memory-overlap',lambda a,b,c:crossing(b,'memory',1),'private generation leaked')
reject('retired-array-before-base-string-overlap',lambda a,b,c:crossing(b,'strings',0),'private generation leaked')
reject('unwritten-new-output-tail-current-read',lambda a,b,c:b['memory'].update({str(b['address_hash_complete']['allocations'][1]['address']+4095):'00'}),'private generation leaked')
reject('unexpected-current-global-change',lambda a,b,c:b['globals'].update(image_read_mode='00000000'),'unexpected changed global')
reject('wrong-actual-packed-output-count',lambda a,b,c:b['output_buffer_state'].update(used=7184),'complete packed-output extent')
reject('changed-actual-packed-output-byte',lambda a,b,c:change_hex(b['output_buffer_state'],'bytes',7187),'complete packed-output extent')
def directory_change(b):
 key=str(int.from_bytes(bytes.fromhex(b['globals']['directory_entries']),'little'));payload=bytearray.fromhex(b['memory'][key]);payload[58*12+8]^=1;b['memory'][key]=payload.hex()
reject('wrong-current-public-directory-size',lambda a,b,c:directory_change(b),'public directory row/tail')
def change_public(b):
 key=str(b['public_padding']['nodes'][0]['address']);payload=bytearray.fromhex(b['memory'][key]);payload[4]^=1;b['memory'][key]=payload.hex()
reject('changed-live-public-record-unused-hash',lambda a,b,c:change_public(b),'current initialized public record')
def change_buckets(b):
 key=str(int.from_bytes(bytes.fromhex(b['globals']['type_table_global']),'little'));payload=bytearray.fromhex(b['memory'][key]);payload[0]^=1;b['memory'][key]=payload.hex()
reject('changed-live-public-buckets',lambda a,b,c:change_buckets(b),'current live public buckets')
reject('old-uninitialized-addresses-schema-retained',lambda a,b,c:b['public_writer_frame'].update(uninitialized_stream_bytes_uncompared=24),'initialized public header/stream schema')
# A raw mutation tests the new decoder's actual post-sort observation, rather
# than only cached JSON. Re-decoding the raw phase does not repeat old controls.
line=next(l for l in events.splitlines()if l.startswith('addresshash-owned sort2-after ') and l.split()[3]=='64');f=line.split();f[-1]=f[-1][:8]+'ff'+f[-1][10:];changed=controller.decode(events.replace(line,' '.join(f),1))
try:compare(original,native,changed)
except AssertionError as error:assert 'post-sort arrays' in str(error);rejected.append(dict(name='raw-mutated-full-postsort-array',diagnostic=str(error)))
else:raise AssertionError('accepted changed raw post-sort bytes')
for field,value,diagnostic in [('typed_pc',native['address_hash_first_callback_observation']['typed_pc']+1,'actual phase entry'),('entry_esp',0,'checkpoint stack delta')]:
 b=copy.deepcopy(native);b['address_hash_first_callback_observation'][field]=value
 try:controller.first_phase(events,b)
 except AssertionError as error:assert diagnostic in str(error),(field,str(error));rejected.append(dict(name='wrong-physical-versus-typed-'+field,diagnostic=str(error)))
 else:raise AssertionError('accepted '+field)
# Focus the new no-mapped-store guard directly with the same actual inputs.
for name,field in [('actual-array',0),('actual-output',1),('actual-method',2)]:
 a=copy.deepcopy(original);b=copy.deepcopy(native);h=copy.deepcopy(historical);c=copy.deepcopy(observations);view=int(next(iter(h['mapped_views'])))
 if field==0:h['address_hash_allocations'][0]['address']=view
 elif field==1:c['allocations'][0]['address']=view
 else:h['address_hash_entry']['stack'][1]=view
 try:controller.mapping_writes(data,a,b,h,b['address_hash_first_callback'],c,b['address_hash_complete'])
 except AssertionError as error:assert 'actual write owner overlaps mapped input' in str(error);rejected.append(dict(name='mapped-input-overlap-'+name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
for name,mutate,reason in [('missing-historical-catalog',lambda h,a:h['mapped_views'].clear(),'catalog inventory'),('wrong-historical-view-extent',lambda h,a:next(iter(h['mapped_views'].values())).update(length=1),'catalog extent'),('late-mapping-API',lambda h,a:a['mapping_calls'].append(dict(unknown=True)),'executed mapping/file API')]:
 h=copy.deepcopy(historical);a=copy.deepcopy(original);mutate(h,a)
 try:controller.mapping_writes(data,a,native,h,previous,observations,native['address_hash_complete'])
 except AssertionError as error:assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
# Initial address-stream storage was own unknown data; final initialized540B
# must be checked independently. Alter only actual entry's pre-init bits.
line=next(l for l in events.splitlines()if l.startswith('addresshash-parent-stack '));f=line.split();payload=bytearray.fromhex(f[-1]);payload[84:108]=bytes(range(24));f[-1]=payload.hex();altered=raw.compare(config,data,events.replace(line,' '.join(f),1),native,artifacts,views);expected=copy.deepcopy(full);expected['fresh_historical_phase']['original_phase_sha256']=altered['fresh_historical_phase']['original_phase_sha256'];assert altered==expected
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,positive_own_preinit_addresses_bits=True,baseline=full['address_hash_complete'],seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/address-hash-complete-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused_prefix=dict(path='cases/hello-debug-dll/evidence/0110-address-hash-first-sort/address-hash-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0110-address-hash-first-sort/address-hash-checks.json'),scope='Prior53 affected generation/first-qsort controls retain110scope. Unchanged prefix predicates freshly pass on actual111 physical-entry/typed checkpoint observations once; current-only mutations call the SAME production validate_current, not a mirror, avoiding repeated old prefix campaigns. New phase entry/prologue and initialized-current-state checks are fresh.'))
(pair/'complete-address-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
