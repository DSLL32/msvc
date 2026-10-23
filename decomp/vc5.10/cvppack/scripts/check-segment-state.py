#!/usr/bin/env python3
"""Complete segment-array and short-argument rejection controls."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0058-add-symbol-segment'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state):return decoder.compare(config,data,events,state,artifacts,views)
compare(native);assert native['function']=='has_symbol_segment' and native['parameters']['segment']==1
assert bytes.fromhex(native['globals']['symbol_segments'])==bytes(510)
rejected=[]
def reject(name,mutation,reason):
    state=copy.deepcopy(native);mutation(state)
    try:compare(state)
    except (AssertionError,KeyError) as error:
        assert (reason in str(error) if reason else type(error) is AssertionError and str(error)==''),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('changed-segment-argument',lambda s:s['parameters'].update(segment=2),'typed ABI arguments differ')
reject('changed-ushort-formal-width',lambda s:s['observed_argument_types'][0].update(width=4),'actual frontier formal width differs')
reject('changed-segment-counter',lambda s:s['globals'].update(symbol_segment_count='0100'),'application global differs')
reject('changed-defined-unused-segment-tail',lambda s:s['globals'].update(symbol_segments='00'*509+'01'),'application global differs')
reject('truncated-segment-array',lambda s:s['globals'].update(symbol_segments='00'*508),'')
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,segment_array_bytes=510,
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'segment-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
