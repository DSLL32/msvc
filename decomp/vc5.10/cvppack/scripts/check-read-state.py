#!/usr/bin/env python3
"""Native-only semantic controls for actual caller-owned PE read buffers."""
import copy
import importlib.util
import json
import time
from state import BINARY

spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0040-widen-subsection-typed-continuation'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
       [('original','events-view-*.bin'),('native','native-view-*.bin')]}
positive=decoder.compare(config,data,events,native,artifacts,views);assert positive['reads_checked']==21
rejected=[]
def reject(name,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:decoder.compare(config,data,events,state,artifacts,views)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
for field,value in [('owner','unknown_owner'),('local','other_local'),('capacity',128),('file',0),('size',63),('entry_position',1),('exit_position',63),('returned',63),('entry_last_error',1),('exit_last_error',1),('bytes','00'*64)]:
    reject('changed-read-'+field,lambda s,f=field,v=value:s['file_reads'][0].__setitem__(f,v))
reject('missing-completed-read',lambda s:s['file_reads'].pop())
reject('reordered-actual-reads',lambda s:s['file_reads'].reverse())
reject('wrong-live-symbol-buffer-alias',lambda s:s['parameters']['symbols'].__setitem__('global_name','type_data'))
reject('wrong-live-symbol-buffer-extent',lambda s:s['parameters']['symbols'].__setitem__('size',115))
reject('changed-live-symbol-buffer-bytes',lambda s:s['parameters']['symbols'].__setitem__('bytes','00'*116))
reject('wrong-symbol-directory-index',lambda s:s['parameters']['entry'].__setitem__('index',2))
report=dict(schema=1,native_only=True,positive_reads=positive['reads_checked'],rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/read-state-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
