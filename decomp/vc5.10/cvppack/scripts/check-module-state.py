#!/usr/bin/env python3
"""Native-only controls for module aliases and owned uninitialized work buffers."""
import copy
import importlib.util
import json
import time
from state import BINARY

spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0038-initialize-single-module-exact-switch'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
       [('original','events-view-*.bin'),('native','native-view-*.bin')]}
positive=decoder.compare(config,data,events,native,artifacts,views)
assert positive['uninitialized_allocations_checked']==6 and positive['aliased_objects_checked']==94 and positive['initialized_module_buffers_checked']==1
rejected=[]
def reject(name,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:decoder.compare(config,data,events,state,artifacts,views)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
def word(state,address,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');state['memory'][str(address)]=raw.hex()
reject('changed-real-malloc-capacity',lambda s:s['image_allocations'][0].__setitem__('size',1))
reject('lost-real-malloc-result',lambda s:s['image_allocations'][0].__setitem__('address',1))
reject('aliased-distinct-work-buffers',lambda s:s['globals'].__setitem__('symbol_data',s['globals']['type_data']))
reject('lost-module-node-link',lambda s:word(s,root(s,'module_list'),44,0))
reject('changed-module-node-id',lambda s:word(s,root(s,'module_list'),0,999))
reject('lost-module-index-alias',lambda s:word(s,root(s,'module_index'),8,0))
reject('wrong-indexed-module',lambda s:word(s,root(s,'module_index'),0,999))
reject('non-NULL-unallocated-segment-slot',lambda s:word(s,root(s,'type_segments'),4,int.from_bytes(bytes.fromhex(s['memory'][str(root(s,'type_segments'))])[:4],'little')))
reject('changed-subsection-allocation-size',lambda s:s['subsection_allocations'][0].__setitem__('size',1))
reject('wrong-subsection-allocation-source',lambda s:s['subsection_allocations'][0].__setitem__('entry_index',0))
reject('wrong-subsection-global-alias',lambda s:s['globals'].__setitem__('segment_map',s['globals']['libraries']))
reject('wrong-module-allocation-size',lambda s:s['module_allocations'][0].__setitem__('size',31))
reject('wrong-module-allocation-source',lambda s:s['module_allocations'][0].__setitem__('entry_index',1))
reject('missing-module-allocation',lambda s:s['module_allocations'].clear())
reject('lost-current-module-buffer',lambda s:s['globals'].__setitem__('current_module_buffer','00000000'))
reject('lost-module-payload-alias',lambda s:word(s,root(s,'current_module_info'),12,0))
reject('changed-module-payload-length',lambda s:word(s,root(s,'current_module_info'),16,29))
reject('changed-module-payload-bytes',lambda s:word(s,root(s,'current_module_buffer'),0,0xffffffff))
reject('lost-directory-resident-pointer',lambda s:word(s,root(s,'directory_entries'),4,0))
reject('wrong-directory-resident-pointer',lambda s:word(s,root(s,'directory_entries'),4,root(s,'symbol_data')))
report=dict(schema=1,native_only=True,positive_objects=94,uninitialized_owned_allocations=6,initialized_module_buffers=1,
    payload_policy='Compare initialized storage only; actual malloc capacities and live aliases cover uninitialized work buffers.',
    rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/module-state-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
