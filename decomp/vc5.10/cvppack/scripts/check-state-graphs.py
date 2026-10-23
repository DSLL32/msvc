#!/usr/bin/env python3
"""Native-only mutation checks for captured target-owned object graph state."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY

spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0010-allocate-file'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
positive=decoder.compare(config,data,events,native);assert positive['aliased_objects_checked']==33
rejected=[]
def reject(name,mutation):
    state=copy.deepcopy(native);mutation(state)
    try:decoder.compare(config,data,events,state)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
def node(state,variable):return int.from_bytes(bytes.fromhex(state['globals'][variable]),'little')
def alter(state,address,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');state['memory'][str(address)]=raw.hex()
reject('changed-page-payload',lambda s:alter(s,node(s,'free_pages'),0,0))
reject('lost-page-link',lambda s:alter(s,node(s,'free_pages'),0x101c,0))
reject('lost-file-object',lambda s:s['memory'].pop(str(node(s,'free_files'))))
def duplicate(s):
    address=node(s,'cached_files');raw=bytes.fromhex(s['memory'][str(address)]);alter(s,address,0,int.from_bytes(raw[4:8],'little'))
reject('wrong-cross-root-file-alias',duplicate)
report=dict(schema=1,native_only=True,positive_objects=33,rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/state-graph-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
