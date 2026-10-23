#!/usr/bin/env python3
"""Native-only semantic controls for the frozen mapped-file-open frontier."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY

spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0018-open-mapped-file'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
positive=decoder.compare(config,data,events,native,artifacts)
assert positive['file_handles_checked']==1
rejected=[]
def reject(name,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:decoder.compare(config,data,events,state,artifacts)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
handle=next(iter(native['file_handles']))
for field,value in [('position',1),('size',1),('type',3),('access_mode',0),('pathname','other.dll'),('sha256','0'*64)]:
    reject('changed-file-'+field,lambda state,f=field,v=value:state['file_handles'][handle].__setitem__(f,v))
reject('changed-create-access',lambda state:state['file_calls'][0].__setitem__('access',0x80000000))
reject('changed-create-sharing',lambda state:state['file_calls'][0].__setitem__('share',1))
reject('changed-create-path',lambda state:state['file_calls'][0].__setitem__('pathname',b'other.dll\0'.hex()))
reject('wrong-handle-ownership',lambda state:state['file_calls'][0].__setitem__('handle',int(handle)+1))
reject('lost-handle-state',lambda state:state['file_handles'].clear())
reject('changed-last-error',lambda state:state.__setitem__('last_error',1))
def lost_registry(state):
    raw=bytearray.fromhex(state['globals']['mapped_file_slots']);raw[0:4]=bytes(4);state['globals']['mapped_file_slots']=raw.hex()
reject('lost-mapped-file-registry-alias',lost_registry)
report=dict(schema=1,native_only=True,positive=positive,rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/file-state-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
