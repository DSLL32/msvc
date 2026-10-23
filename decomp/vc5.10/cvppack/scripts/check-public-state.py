#!/usr/bin/env python3
"""Check initialized public nodes, bump ownership and the new type signature."""
import copy
import importlib.util
import json
import time
from state import BINARY, sha
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0049-read-module-types'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
positive=decoder.compare(config,data,events,native,artifacts,views);assert positive['aliased_objects_checked']==99 and positive['reads_checked']==23
rejected=[]
def root(s,name):return int.from_bytes(bytes.fromhex(s['globals'][name]),'little')
def word(s,address,offset,value):
 raw=bytearray.fromhex(s['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');s['memory'][str(address)]=raw.hex()
def reject(name,mutate):
 s=copy.deepcopy(native);mutate(s)
 try:decoder.compare(config,data,events,s,artifacts,views)
 except (AssertionError,KeyError):rejected.append(name)
 else:raise AssertionError('accepted '+name)
table=bytes.fromhex(native['memory'][str(root(native,'type_table_global'))]);nodes=[int.from_bytes(table[i:i+4],'little') for i in range(0,len(table),4) if any(table[i:i+4])];assert len(nodes)==4
for field,offset in [('hash',4),('offset',8),('name-and-segment',12),('record',16),('name-bytes',32)]:
 reject('changed-public-'+field,lambda s,o=offset:word(s,nodes[0],o,0xffffffff))
def changed_padding(s):
 for node in nodes:
  raw=bytearray.fromhex(s['memory'][str(node)]);name=int.from_bytes(raw[12:14],'little');end=16+name+1+raw[16+name]
  if end<len(raw):raw[end]^=1;s['memory'][str(node)]=raw.hex();return
 raise AssertionError('case has no padding')
reject('changed-public-padding',changed_padding)
reject('wrong-arena-cursor',lambda s:s['globals'].__setitem__('public_arena_cursor',(root(s,'public_arena_cursor')+4).to_bytes(4,'little').hex()))
reject('wrong-arena-remaining',lambda s:s['globals'].__setitem__('public_arena_remaining','00000000'))
reject('lost-public-bucket-node',lambda s:word(s,root(s,'type_table_global'),next(i for i in range(0,len(table),4) if any(table[i:i+4])),0))
reject('wrong-public-node-allocator',lambda s:s['checked_allocations'][-1].__setitem__('address',1))
reject('wrong-public-node-arena-size',lambda s:s['checked_allocations'][-1].__setitem__('size',8121))
reject('changed-live-type-signature',lambda s:word(s,root(s,'line_data'),0,1))
reject('wrong-live-type-page-alias',lambda s:s['globals'].__setitem__('line_data',s['globals']['symbol_data']))
reject('wrong-type-stream-extent',lambda s:s['parameters'].__setitem__('size',12323))
reject('wrong-precompiled-flag',lambda s:s['parameters'].__setitem__('precompiled',1))
report=dict(schema=1,native_only=True,public_nodes=4,rejected=rejected,
 provenance=dict(checker_sha256=sha(BINARY/'scripts/check-public-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),contract_sha256=sha(pair/'original/configuration.json'),data_sha256=sha(pair/'original/data.json')),
 seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/public-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
