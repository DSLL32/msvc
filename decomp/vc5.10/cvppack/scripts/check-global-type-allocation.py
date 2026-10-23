#!/usr/bin/env python3
"""Actual global pointer-block allocation and arena-copy boundary controls."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0068-allocate-global-type-inline-padding'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['arena']==dict(null=False,global_name='type_large_arena') and baseline['parameters']['size']==24
assert native['globals']['next_global_type']=='00100000' and native['globals']['type_table_used']=='00020000'
block=int.from_bytes(bytes.fromhex(native['globals']['global_type_blocks'])[:4],'little')
assert native['checked_allocations'][-1]==dict(address=block,size=2048)
# No pointer slots have been initialized before the first copy. Those native
# allocation bytes are owned, not an original byte-for-byte replay obligation.
positive=copy.deepcopy(native);positive['memory'][str(block)]=('a5'*2048);compare(positive)
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('wrong-copy-arena',lambda s:s['parameters']['arena'].update(global_name='type_arena'),'typed ABI arguments differ')
reject('wrong-copy-size',lambda s:s['parameters'].update(size=23),'typed ABI arguments differ')
reject('changed-full-copy-input',lambda s:s['parameters']['data'].update(bytes='00'*24),'typed ABI arguments differ')
reject('wrong-real-arena-pointee',lambda s:s['observed_argument_types'][0].update(pointee_width=20),'actual frontier pointee extent differs')
def blockword(state,offset,value):
    raw=bytearray.fromhex(state['globals']['global_type_blocks']);raw[offset:offset+4]=value.to_bytes(4,'little');state['globals']['global_type_blocks']=raw.hex()
reject('wrong-pointer-block-owner',lambda s:blockword(s,0,block+4),'global type block allocation/alias/capacity differs')
reject('nonzero-unused-static-block-slot',lambda s:blockword(s,4,block),'unused global type block slot is not zero')
reject('wrong-actual-checked-capacity',lambda s:s['checked_allocations'][-1].update(size=2047),'checked allocator request/result differs')
for name,value in [('type_table_used',1024),('next_global_type',0x1001)]:
    item=next(row for row in data if row['name']==name);replacement=value.to_bytes(4,'little').hex()
    old='memory %08x '%item['original'];lines=events.splitlines();index=max(i for i,line in enumerate(lines) if line.startswith(old));lines[index]=old+replacement
    reject('unsupported-'+name,lambda s,n=name,v=replacement:s['globals'].update({n:v}),'global type block contents need an observed initialized-prefix contract','\n'.join(lines)+'\n')
with tempfile.TemporaryDirectory(prefix='cvpack-arena-copy-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config)
    row=next(row for row in changed['functions'] if row['name']=='copy_type_arena');row['parameters'][0]['native_pointee_width']=20
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
    assert result.returncode and 'actual pointer formal pointee extent differs' in diagnostic,diagnostic
    actual=dict(returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py'))
report=dict(schema=1,native_only=True,count=len(rejected)+1,rejected=rejected,actual_gdb_rejection=actual,native_own_uninitialized_block_positive=True,actual_capacity=2048,defined_slots=0,copy_arguments=baseline['parameters'],original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'global-type-allocation-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
