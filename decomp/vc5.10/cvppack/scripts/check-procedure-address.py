#!/usr/bin/env python3
"""Procedure-scope state and ModuleInfo/ushort/offset frontier ABI."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
from state import BINARY, ROOT, sha
from binary import PE

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0063-rewrite-procedure32-symbol'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in
    [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['segment']==1 and baseline['parameters']['offset']==0
assert native['globals']['public_scope_depth']=='01000000'
rejected=[]
def reject(name,mutation,reason,raw=events):
    changed=copy.deepcopy(native);mutation(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error))
        rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('wrong-segment',lambda s:s['parameters'].update(segment=2),'typed ABI arguments differ')
reject('wrong-code-offset',lambda s:s['parameters'].update(offset=1),'typed ABI arguments differ')
reject('wrong-module-identity',lambda s:s['parameters']['info'].update(address=s['parameters']['info']['address']+4),'object alias differs')
reject('wrong-module-pointee-extent',lambda s:s['observed_argument_types'][0].update(pointee_width=44),'actual frontier pointee extent differs')
reject('wrong-short-parameter-width',lambda s:s['observed_argument_types'][1].update(width=4),'actual frontier formal width differs')
reject('missing-scope-increment',lambda s:s['globals'].update(public_scope_depth='00000000'),'application global differs')
original=decoder.decode(events);word=original['stack'][2];assert word&0xffff==1
lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('memory %08x '%original['esp']))
prefix,payload=lines[i].rsplit(' ',1);assert payload[16:24]==struct.pack('<I',word).hex()
lines[i]=prefix+' '+payload[:16]+struct.pack('<I',word^0x12340000).hex()+payload[24:]
compare(native,'\n'.join(lines)+'\n')
lines[i]=prefix+' '+payload[:16]+struct.pack('<I',word^1).hex()+payload[24:]
reject('wrong-original-short-low-word',lambda s:None,'typed ABI arguments differ','\n'.join(lines)+'\n')
provider=PE(ROOT/config['target']['path']);assert provider.read(0x40afb0,5)==bytes.fromhex('668b7c2418')
with tempfile.TemporaryDirectory(prefix='cvpack-procedure-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config)
    row=next(row for row in changed['functions'] if row['name']=='module_contains_address');row['parameters'][0]['native_pointee_width']=44
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),
        '--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],
        cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'))
    assert result.returncode and 'actual pointer formal pointee extent differs' in diagnostic,diagnostic
    actual=dict(returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py'))
report=dict(schema=1,native_only=True,count=len(rejected)+1,rejected=rejected,actual_gdb_module_rejection=actual,
    actual_formals=native['observed_argument_types'],ushort_load=dict(address=0x40afb0,bytes='668b7c2418'),
    dead_ushort_high_word_change_allowed=True,original_receipt_sha256=sha(pair/'original/receipt.json'),
    native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'procedure-address-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(dict(rejected=report['count'],seconds=report['seconds']),indent=2))
