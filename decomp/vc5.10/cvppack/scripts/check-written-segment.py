#!/usr/bin/env python3
"""The emitted segment symbol is defined; its remaining allocation is not."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY, sha

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--pair',type=Path,default=BINARY/'cases/hello-debug-dll/evidence/0061-write-segment-record')
parser.add_argument('--defined-prefix',type=int,default=16)
args=parser.parse_args();start=time.perf_counter();pair=args.pair.resolve();defined=args.defined_prefix
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state):return decoder.compare(config,data,events,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']=={}
allocation=next(row for row in native['image_allocations'] if row.get('owner')=='rewrite_module_symbols')
address=allocation['address'];raw=bytes.fromhex(native['memory'][str(address)])
assert raw[:16]==bytes.fromhex('020000000a0005000000000001000000')
assert 16<=defined<=allocation['size']
assert int.from_bytes(bytes.fromhex(native['globals']['symbol_output']),'little')==address+defined
rejected=[]
tests=[('length',4),('kind',6),('offset',8),('segment',12),('padding',14)]
if defined>26:tests.extend([('copied-record',26),('final-defined-byte',defined-1)])
for label,offset in tests:
    changed=copy.deepcopy(native);raw=bytearray.fromhex(changed['memory'][str(address)]);raw[offset]^=1
    changed['memory'][str(address)]=raw.hex()
    try:compare(changed)
    except AssertionError as error:
        assert 'initialized symbol output prefix differs' in str(error),str(error)
        rejected.append(dict(name=label,diagnostic=str(error)))
    else:raise AssertionError('accepted changed segment '+label)
changed=copy.deepcopy(native);raw=bytearray.fromhex(changed['memory'][str(address)])
raw[defined:]=bytes(value^0xff for value in raw[defined:]);changed['memory'][str(address)]=raw.hex();compare(changed)
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,initialized_prefix=defined,
    native_owned_tail_change_allowed=True,real_empty_callback_formals=native['observed_argument_types'],
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'written-segment-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
