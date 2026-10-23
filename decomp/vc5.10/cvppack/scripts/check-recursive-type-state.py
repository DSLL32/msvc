#!/usr/bin/env python3
"""Recursive-record entry ownership and actual GDB formal-schema controls."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0080-resolve-recursive-type'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']==dict(record=dict(root='type_records',index=332))
original=decoder.decode(events);rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
def ecx(value):
    lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[3]='%08x'%value;lines[i]=' '.join(fields);return '\n'.join(lines)+'\n'
reject('wrong-native-local-record',lambda s:s['parameters']['record'].update(index=333),'typed ABI arguments differ')
reject('wrong-original-local-record',lambda s:None,'typed ABI arguments differ',ecx(original['registers']['ecx']+32))
reject('original-misaligned-record',lambda s:None,'argument is not an owned original local type record',ecx(original['registers']['ecx']+1))
reject('wrong-observed-pointee-width',lambda s:s['observed_argument_types'][0].update(pointee_width=4),'actual frontier pointee extent differs')
def depth(state):
    raw=int.from_bytes(bytes.fromhex(state['globals']['type_recursion_depth']),'little')^1
    state['globals']['type_recursion_depth']=raw.to_bytes(2,'little').hex()
reject('lost-caller-recursion-depth',depth,'application global differs')
actual=[]
for name in ['pointee-width','omitted-formal']:
    with tempfile.TemporaryDirectory(prefix='cvpack-recursive-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(row for row in changed['functions'] if row['name']=='hash_recursive_type')
        if name=='pointee-width':row['parameters'][0]['native_pointee_width']=4;reason='actual pointer formal pointee extent differs'
        else:row['parameters']=[];reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
        assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=baseline['parameters'],original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'recursive-type-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
