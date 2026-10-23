#!/usr/bin/env python3
"""Source-module read/storage and next fastcall argument controls."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY, sha


start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0054-read-module-source-lines'
config=json.loads((pair/'original/configuration.json').read_text())
data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
baseline=decoder.compare(config,data,events,native,artifacts,views)
assert baseline['parameters']==dict(index=0)
address=int.from_bytes(bytes.fromhex(native['globals']['module_data']),'little')
assert len(bytes.fromhex(native['memory'][str(address)]))==256
tests=[]
def reject(name,mutation,reason,raw=events):
    state=copy.deepcopy(native);mutation(state)
    try:decoder.compare(config,data,raw,state,artifacts,views)
    except (AssertionError,KeyError) as error:
        assert reason in str(error), ('wrong rejection guard',name,str(error))
        tests.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
def change(state):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[128]^=1
    state['memory'][str(address)]=raw.hex()
reject('changed-source-module-byte',change,'initialized subsection buffer bytes differ')
reject('incomplete-source-module-capacity',lambda s:s['memory'].__setitem__(str(address),s['memory'][str(address)][:-2]),'incomplete read-buffer capacity observation')
reject('wrong-next-ECX-type-index',lambda s:s['parameters'].update(index=1),'typed ABI arguments differ')
reject('changed-source-read-result',lambda s:s['file_reads'][-1].update(returned=255),'actual read caller/extent/position/result/initialized output differs')
assert ' 00405408 ' in events
reject('unknown-original-source-read-caller',lambda s:None,'unknown actual original read caller',events.replace(' 00405408 ',' 00405409 '))
report=dict(schema=1,native_only=True,source_module_bytes=256,rejected=tests,count=len(tests),
    next_original_register='ecx',next_actual_formals=native['observed_argument_types'],
    decoder_sha256=sha(BINARY/'scripts/raw-state.py'),checker_sha256=sha(Path(__file__)),
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    seconds=time.perf_counter()-start)
(BINARY/'cases/hello-debug-dll/source-module-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=len(tests),seconds=report['seconds']),indent=2))
