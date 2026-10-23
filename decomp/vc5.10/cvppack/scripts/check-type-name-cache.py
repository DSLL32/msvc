#!/usr/bin/env python3
"""Owned cache nodes/count-prefixed names, segment aliases and full next ABI."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0059-has-symbol-segment'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['cached_type_names_checked']==47
root=int.from_bytes(bytes.fromhex(native['globals']['type_references']),'little')
heads=bytes.fromhex(native['memory'][str(root)])
offset=next(i for i in range(0,2048,4) if any(heads[i:i+4]))
node=int.from_bytes(heads[offset:offset+4],'little');record=bytes.fromhex(native['memory'][str(node)])
name=int.from_bytes(record[:4],'little');payload=int.from_bytes(record[4:8],'little')
def write(state,address,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+len(value)]=value
    state['memory'][str(address)]=raw.hex()
rejected=[]
def reject(label,mutation,reason,raw=events):
    state=copy.deepcopy(native);mutation(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',label,str(error))
        rejected.append(dict(name=label,diagnostic=str(error)))
    else:raise AssertionError('accepted '+label)
reject('changed-counted-name-content',lambda s:write(s,name,1,b'!'),'type-name counted string bytes differ')
reject('wrong-counted-name-capacity',lambda s:write(s,name,0,b'\xff'),'incomplete type-name counted string observation')
reject('truncated-counted-name',lambda s:s['memory'].__setitem__(str(name),s['memory'][str(name)][:-2]),'incomplete type-name counted string observation')
reject('changed-type-index',lambda s:write(s,node,8,b'\xff\xff\x00\x00'),'type-name index/reserved fields differ')
reject('changed-node-reserved-field',lambda s:write(s,node,12,b'\x01\x00\x00\x00'),'type-name index/reserved fields differ')
reject('wrong-type-segment-offset',lambda s:write(s,node,4,(payload+4).to_bytes(4,'little')),'type-name record segment alias differs')
reject('missing-live-record-pointer',lambda s:write(s,node,4,bytes(4)),'type-name record NULL state differs')
reject('cyclic-node-link',lambda s:write(s,node,16,node.to_bytes(4,'little')),'type-name node NULL/identity/chain differs')
reject('missing-bucket-head',lambda s:write(s,root,offset,bytes(4)),'type-name node NULL/identity/chain differs')
reject('wrong-node-allocation-size',lambda s:next(row for row in s['cleared_allocations'] if row['address']==node).update(size=24),'cleared allocation request/result differs')
reject('changed-completed-alignment-count',lambda s:s['globals'].update(module_symbol_count='6302'),'application global differs')
reject('changed-record-pointee-width',lambda s:s['observed_argument_types'][1].update(pointee_width=8),'actual frontier pointee extent differs')
reject('missing-next-formal',lambda s:s['observed_argument_names'].pop(),'actual frontier formal-argument inventory differs')
reject('wrong-reference-counter-alias',lambda s:s['parameters']['references'].update(global_name='module_symbol_count'),'typed ABI arguments differ')
original=decoder.decode(events)
global_address=next(row['original'] for row in data if row['name']=='type_references')
original_root=int.from_bytes(original['memory'][global_address],'little')
original_node=int.from_bytes(original['memory'][original_root][offset:offset+4],'little')
original_name=int.from_bytes(original['memory'][original_node][:4],'little')
lines=events.splitlines();prefix='memory %08x '%original_name
changed=[]
for line in lines:
    if line.startswith(prefix):
        value=bytearray.fromhex(line[len(prefix):]);value[1]^=1;line=prefix+value.hex()
    changed.append(line)
reject('changed-raw-original-name',lambda s:None,'type-name counted string bytes differ','\n'.join(changed)+'\n')
reject('missing-raw-original-name-observation',lambda s:None,str(original_name),
    '\n'.join(line for line in lines if not line.startswith(prefix))+'\n')
actual=[]
for label,mutation,reason in [
    ('missing-actual-next-formal',lambda r:r['parameters'].pop(),'incomplete frontier ABI contract'),
    ('wrong-actual-directory-pointee',lambda r:r['parameters'][1].update(element_size=8),'actual pointer formal pointee extent differs')]:
    with tempfile.TemporaryDirectory(prefix='cvpack-cache-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config)
        mutation(next(row for row in changed['functions'] if row['name']=='rewrite_new_lines'))
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
            '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
            cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
        assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=label,returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,
    cache_nodes=47,buckets=512,node_bytes=20,complete_counted_names=True,segment_owned_record_aliases=True,
    actual_next_formals=native['observed_argument_types'],original_receipt_sha256=sha(pair/'original/receipt.json'),
    native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),
    decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),seconds=time.perf_counter()-start)
(pair/'type-name-cache-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
