#!/usr/bin/env python3
"""Actual payload ownership/bytes and interning's complete cached-free sites."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0066-intern-type'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['data']['record_indices']==[12]
assert len(bytes.fromhex(baseline['parameters']['data']['bytes']))==24
rejected=[]
def reject(name,mutation,reason,raw=events):
    changed=copy.deepcopy(native);mutation(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('wrong-payload-segment-offset',lambda s:s['parameters']['data'].update(offset=605),'typed ABI arguments differ')
reject('wrong-payload-record-owner',lambda s:s['parameters']['data'].update(record_indices=[13]),'typed ABI arguments differ')
reject('changed-complete-payload-argument',lambda s:s['parameters']['data'].update(bytes='00'*24),'typed ABI arguments differ')
reject('incomplete-payload-argument',lambda s:s['parameters']['data'].update(bytes=s['parameters']['data']['bytes'][:-2]),'typed ABI arguments differ')
reject('wrong-actual-byte-pointee',lambda s:s['observed_argument_types'][0].update(pointee_width=4),'actual frontier pointee extent differs')
root=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little')
def wrong_record(state):
    raw=bytearray.fromhex(state['memory'][str(root)]);pointer=int.from_bytes(raw[12*32:12*32+4],'little')
    raw[12*32:12*32+4]=(pointer+4).to_bytes(4,'little');state['memory'][str(root)]=raw.hex()
reject('payload-pointer-disagrees-with-owner',wrong_record,'type record segment/payload offset alias differs')
base=int.from_bytes(bytes.fromhex(native['globals']['line_data']),'little')
def wrong_bytes(state):
    raw=bytearray.fromhex(state['memory'][str(base)]);raw[604+12]^=1;state['memory'][str(base)]=raw.hex()
reject('changed-owned-segment-payload',wrong_bytes,'initialized type segment bytes differ')
original=decoder.decode(events);lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '))
fields=lines[i].split();fields[3]='%08x'%(int(fields[3],16)+1);lines[i]=' '.join(fields)
reject('unowned-original-payload-interior',lambda s:None,'original type payload has no observed local-record/segment owner','\n'.join(lines)+'\n')

audit=json.loads((pair/'original/audit.json').read_text());report=audit['graph']['functions']['@intern_type@4']['register_imports']['original']
free=dict(dll='msvcrt.dll',name='free')
assert report['resolved_register_calls']=={str(address):free for address in (0x40b395,0x40b39e)}
assert report['unresolved_register_calls']==[] and report['other_unresolved_calls']==[]
with tempfile.TemporaryDirectory(prefix='cvpack-intern-contract-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);metadata=json.loads((pair/'original/graph-metadata.json').read_text())
    contract=next(row for row in metadata['register_imports']['preservation_contracts'] if row['target'].get('import')==free)
    contract['preserves'].remove('esi');bad=directory/'metadata.json';bad.write_text(json.dumps(metadata))
    command=list(audit['graph_command']);command[command.index('--metadata')+1]=str(bad)
    result=subprocess.run(command,cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic=result.stderr.decode();assert result.returncode and '@intern_type@4' in diagnostic,diagnostic
    rejected.append(dict(name='missing-free-ESI-preservation',diagnostic=diagnostic))
with tempfile.TemporaryDirectory(prefix='cvpack-payload-byte-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config)
    row=next(row for row in changed['functions'] if row['name']=='hash_type_record');row['parameters'][0]['native_pointee_width']=4
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
        '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
        cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
    assert result.returncode and 'actual pointer formal pointee extent differs' in diagnostic,diagnostic
    actual=dict(returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py'))
report=dict(schema=1,native_only=True,count=len(rejected)+1,rejected=rejected,actual_gdb_byte_rejection=actual,
    payload=baseline['parameters']['data'],cached_free_sites=[0x40b395,0x40b39e],no_unknown_register_calls=True,
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'type-payload-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
