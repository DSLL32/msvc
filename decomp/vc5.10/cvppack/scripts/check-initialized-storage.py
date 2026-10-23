#!/usr/bin/env python3
"""Native-only semantic controls for initialized type and directory allocations."""
import copy
import importlib.util
import json
import time
from state import BINARY

spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0032-compare-directory-entries'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
       [('original','events-view-*.bin'),('native','native-view-*.bin')]}
positive=decoder.compare(config,data,events,native,artifacts,views)
rejected=[]
def reject(name,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:decoder.compare(config,data,events,state,artifacts,views)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
def change(state,address,offset=0):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset]^=1;state['memory'][str(address)]=raw.hex()
reject('changed-cleared-type-table-byte',lambda s:change(s,root(s,'type_table_primary'),8192))
reject('changed-sorted-directory-byte',lambda s:change(s,root(s,'directory_entries'),839))
reject('lost-initialized-directory-extent',lambda s:s['memory'].__setitem__(str(root(s,'directory_entries')),'00'*12))
reject('changed-directory-allocation-capacity',lambda s:s['directory_allocation'].__setitem__('size',840))
reject('wrong-directory-allocation-ownership',lambda s:s['directory_allocation'].__setitem__('address',1))
reject('aliased-distinct-type-tables',lambda s:s['globals'].__setitem__('type_table_global',s['globals']['type_table_primary']))
def nested(s):
    raw=bytes.fromhex(s['memory'][str(root(s,'empty_type_record'))]);change(s,int.from_bytes(raw[:4],'little'),4)
reject('changed-nested-empty-type-record',nested)
reject('changed-empty-type-flags',lambda s:change(s,root(s,'empty_type_record'),29))
report=dict(schema=1,native_only=True,positive_objects=positive['aliased_objects_checked'],rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/initialized-storage-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
