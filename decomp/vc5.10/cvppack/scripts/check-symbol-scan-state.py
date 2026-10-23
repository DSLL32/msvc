#!/usr/bin/env python3
"""First symbol scan, alignment counters and real ushort call ABI controls."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0057-scan-module-symbols'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state):return decoder.compare(config,data,events,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['segment']==1
assert native['globals']['module_symbol_count']=='0600'
assert native['globals']['module_reference_count']==native['globals']['symbol_segment_count']=='0000'
rejected=[]
def reject(name,mutation,reason):
    state=copy.deepcopy(native);mutation(state)
    try:compare(state)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('wrong-segment-number',lambda s:s['parameters'].__setitem__('segment',2),'typed ABI arguments differ')
reject('wrong-symbol-counter-alias',lambda s:s['parameters']['symbols'].update(global_name='module_reference_count'),'typed ABI arguments differ')
reject('incorrect-actual-short-width',lambda s:s['observed_argument_types'][0].update(width=4),'actual frontier formal width differs')
reject('incorrect-actual-output-width',lambda s:s['observed_argument_types'][1].update(pointee_width=4),'actual frontier pointee extent differs')
for name,value in [('module_symbol_count','0500'),('module_reference_count','0100'),('symbol_segment_count','0100')]:
    reject('changed-'+name,lambda s,n=name,v=value:s['globals'].__setitem__(n,v),'application global differs')
with tempfile.TemporaryDirectory(prefix='cvpack-symbol-short-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config)
    row=next(row for row in changed['functions'] if row['name']=='add_symbol_segment');row['parameters'][0]['native_width']=4
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
        '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
        cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
    assert result.returncode and 'actual frontier formal width differs' in diagnostic,diagnostic
    actual=dict(returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py'))
report=dict(schema=1,native_only=True,count=len(rejected)+1,rejected=rejected,actual_gdb_short_rejection=actual,
    original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),
    checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'symbol-scan-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
