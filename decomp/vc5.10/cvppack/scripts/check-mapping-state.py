#!/usr/bin/env python3
"""Native-only semantic controls for a surviving, fully observed mapped view."""
import copy
import importlib.util
import json
import time
from state import BINARY

spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0019-create-mapping-typed-frontier'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
       [('original','events-view-*.bin'),('native','native-view-*.bin')]}
positive=decoder.compare(config,data,events,native,artifacts,views)
assert positive['mapped_views_checked']==1
source=artifacts['hello.dll'];original_input=(BINARY/'cases/hello-debug-dll/preparation-0001/hello.dll').read_bytes()
assert len(source)==2097152 and source[:len(original_input)]==original_input and not any(source[len(original_input):])
rejected=[]
def reject(name,mutate):
    state=copy.deepcopy(native);observed=copy.deepcopy(views);mutate(state,observed)
    try:decoder.compare(config,data,events,state,artifacts,observed)
    except (AssertionError,KeyError):rejected.append(name)
    else:raise AssertionError('accepted '+name)
view=next(iter(native['mapped_views']))
for field,value in [('length',4096),('permissions','r--p'),('offset',4096),('pathname','other.dll'),('call_index',0),('sha256','0'*64)]:
    reject('changed-view-'+field,lambda s,v,f=field,x=value:s['mapped_views'][view].__setitem__(f,x))
reject('wrong-mapping-source',lambda s,v:s['mapping_calls'][0]['args'].__setitem__(0,999))
reject('wrong-mapping-protection',lambda s,v:s['mapping_calls'][0]['args'].__setitem__(2,2))
reject('wrong-registry-ownership',lambda s,v:s['mapping_calls'][0]['registry'].__setitem__('source_owned',False))
reject('wrong-closed-mapping-identity',lambda s,v:s['mapping_calls'][2]['args'].__setitem__(0,999))
reject('missing-mapping-close',lambda s,v:s['mapping_calls'].pop())
reject('lost-surviving-view',lambda s,v:s['mapped_views'].clear())
def change(s,v):
    name=s['mapped_views'][view]['file'];raw=bytearray(v['native'][name]);raw[-1]^=1;v['native'][name]=bytes(raw)
    import hashlib
    s['mapped_views'][view]['sha256']=hashlib.sha256(raw).hexdigest()
reject('changed-full-capacity-tail-with-refreshed-hash',change)
reject('truncated-original-full-view',lambda s,v:v['original'].__setitem__('events-view-1.bin',bytes(4096)))
report=dict(schema=1,native_only=True,positive=positive,original_prefix_bytes=len(original_input),zero_extension_bytes=len(source)-len(original_input),rejected=rejected,seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/mapping-state-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(positive_views=positive['mapped_views_checked'],rejections=len(rejected),seconds=report['seconds']),indent=2))
