#!/usr/bin/env python3
"""Observed DWORD lookup returns and actual recursive comparator arena inputs."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0086-lookup-global-type-arena-contract'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['lookup_type_calls_checked']==2
assert baseline['parameters']['record']==dict(root='type_records',index=166)
assert baseline['parameters']['index']==4262 and baseline['parameters']['other_index']==4292
assert baseline['parameters']['other_data']['record_indices']==[155]
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('changed-full-dword-output',lambda s:s['lookup_type_calls'][0].update(output=0x1000000),'global-type lookup ABI/output sequence differs')
reject('changed-input-index',lambda s:s['lookup_type_calls'][0].update(index=204),'global-type lookup ABI/output sequence differs')
reject('lost-call',lambda s:s['lookup_type_calls'].pop(),'global-type lookup call inventory differs')
reject('changed-returned-record',lambda s:s['lookup_type_calls'][0].update(result=s['lookup_type_calls'][0]['result']+32),'global-type lookup returned record identity differs')
reject('invented-recursive-alias',lambda s:s['lookup_type_calls'][0].update(parent_argument_alias=True),'global-type lookup ABI/output sequence differs')
reject('unfinished-native-call',lambda s:s.update(lookup_type_active_depth=1),'unfinished global-type lookup observation')
last=events.rfind('lookup-type-return ');end=events.index('\n',last)
reject('missing-original-return',lambda s:None,'unfinished global-type lookup observation',events[:last]+events[end+1:])
reject('truncated-original-output',lambda s:None,'incomplete global-type lookup observation',events+'lookup-type-failed\n')
for field,value in [('arena','type_arena'),('offset',0),('record_indices',[156]),('index',4293),('bytes','0000')]:
    reject('changed-comparator-arena-'+field,lambda s,f=field,v=value:s['parameters']['other_data'].update({f:v}),'typed ABI arguments differ')
# Native-only independent examples cover recursion's parent-argument-slot reuse,
# a full DWORD zero overwrite, and sentinel handling. They are not extra originals.
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/lookup-type-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-lookup-type-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'lookup-type-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
    assert result.stdout==b'recursive-parent-output direct-zero sentinel-zero\n'
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,observed_lookup_calls=2,case_recursion_observed=False,parameters=baseline['parameters'],original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),unit=dict(scope='Independent native examples; not original branch coverage.',inputs=inputs,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'lookup-type-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
