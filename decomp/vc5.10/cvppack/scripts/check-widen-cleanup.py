#!/usr/bin/env python3
"""Two scratch CRT lifetimes, same-generation prefix and finite consumer tests."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0097-free-widen-buffers'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
def module(name):
    spec=importlib.util.spec_from_file_location(name.replace('-','_'),BINARY/'scripts'/(name+'.py'));value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value
decoder=module('raw-state');free_state=module('main-free-state')
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);original=decoder.decode(events);assert baseline['main_free_lifetimes']['calls']==7 and baseline['main_free_lifetimes']['retired_allocations']==6
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('missing-scratch-free',lambda s:s['main_free_calls'].pop(),'emitted scratch lifetime lacks actual free return')
for field,value,reason in [('address',0,'emitted scratch lifetime lacks actual free return'),('index',0,'main free actual sequence/source differs'),('owner','cvpack_main','main free native cdecl return differs'),('exit_esp',0,'main free native cdecl return differs'),('exit_last_error',123,'main free observable LastError differs')]:
    reject('wrong-scratch-'+field,lambda s,f=field,v=value:s['main_free_calls'][5].update({f:v}),reason)
def byte(state,index,offset):
    call=state['main_free_calls'][index];raw=bytearray.fromhex(call['before']);raw[offset]^=1;call['before']=raw.hex()
# The last emit wrote96B/two fixups; earlier writes to this SAME allocation
# generation initialized103B/four fixups. These tests detect last-only credit.
reject('changed-earlier-same-generation-byte102',lambda s:byte(s,5,102),'main free initialized pre-free bytes differ')
reject('changed-earlier-same-generation-fixup-byte31',lambda s:byte(s,6,31),'main free initialized pre-free bytes differ')
reject('truncated-scratch-capacity',lambda s:s['main_free_calls'][6].update(before=s['main_free_calls'][6]['before'][:-2]),'main free incomplete pre-free capacity')
for index in (5,6):
    address=native['main_free_calls'][index]['address']
    reject('retired-scratch-overlapping-current-'+str(index),lambda s,a=address:s['memory'].update({str(a-4):'00'*8}),'released payload was captured as current state')
reject('unknown-next-consumer',lambda s:s.update(frontier_caller='free_widen_buffers'),'unmodeled target consumer after main free')
match=re.search(r'^main-free-call 00405249 [0-9a-f]{8} [0-9]+ ([0-9a-f]{8})$',events,re.M);assert match
begin,end=match.span(1);reject('original-wrong-second-scratch-provider',lambda s:None,'main free actual canonical provider differs',events[:begin]+'00000000'+events[end:])
# The inverse property is equally important: bytes never written by any emit
# in this generation remain allocator-owned uninitialized values.
changed=copy.deepcopy(native);byte(changed,5,103);byte(changed,6,32);compare(changed)
# Isolate the lifetime controller's generation filter: if no recorded emission
# belongs to the freed allocation, it must not credit writes from other buffers.
image_aliases={};image_sizes={}
for key in ('image_allocations','checked_allocations','cleared_allocations'):
    for old,new in zip(original[key],native[key]):image_aliases[old['address']]=new['address'];image_sizes[old['address']]=old['size']
changed=copy.deepcopy(original);address=changed['main_free_calls'][5]['address']
for call in changed['widen_emit_calls']:
    description=bytearray(call['after'][call['description']])
    if int.from_bytes(description[8:12],'little')==address:description[8:12]=(address+0x100000).to_bytes(4,'little')
    call['after'][call['description']]=bytes(description)
try:free_state.validate(config,data,changed,native,image_aliases,image_sizes,[])
except AssertionError as error:
    assert 'scratch free has no same-generation initialized emit writes' in str(error),str(error);rejected.append(dict(name='isolated-no-writes-for-freed-generation',diagnostic=str(error)))
else:raise AssertionError('credited other allocation generation')
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,lifetimes=baseline['main_free_lifetimes'],actual_emit_lifetimes=21,
 same_generation_initialized_scratch=dict(output_bytes=103,fixup_records=4,fixup_bytes=32),arbitrary_uninitialized_pre_free_scratch_positive=True,
 scope='New twofree wrapper/state/generation paths freshly checked; unchanged20main-free overlap/source/ABI controls and2scope branches reused from96 with source/proof hashes.',
 reused=dict(path='cases/hello-debug-dll/evidence/0096-enter-symbol-scope-lifetimes/lifetime-checks-v2.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0096-enter-symbol-scope-lifetimes/lifetime-checks-v2.json')),
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),audit_sha256=sha(pair/'current-audit/audit.json'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),lifetime_validator_sha256=sha(BINARY/'scripts/main-free-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),checker_sha256=sha(Path(__file__)),seconds=time.perf_counter()-start)
(pair/'cleanup-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=report['seconds'])))
