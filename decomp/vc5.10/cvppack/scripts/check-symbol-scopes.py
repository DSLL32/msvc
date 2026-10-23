#!/usr/bin/env python3
"""Actual scope-link writes, full initialized buffer and WORD module ABI."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from binary import PE
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0088-link-symbol-scopes-buffer-contract'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['module']==2
original=decoder.decode(events);payload=bytes.fromhex(native['parameters']['buffer']['bytes']);expected={8:108,112:0,116:300,120:304,308:0,312:528,316:532,536:0,540:648,544:0}
for offset,value in expected.items():assert int.from_bytes(payload[offset:offset+4],'little')==value
allocation=next(row for row in native['image_allocations'] if row.get('owner')=='rewrite_module_symbols');base=allocation['address']
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
def change_link(state,offset):
    raw=bytearray.fromhex(state['memory'][str(base)]);raw[offset]^=1;state['memory'][str(base)]=raw.hex()
for name,offset in [('root',8),('parent',112),('end',116),('next',120)]:reject('changed-actual-scope-'+name,lambda s,o=offset:change_link(s,o),'initialized symbol output prefix differs')
reject('wrong-module-low-word',lambda s:s['parameters'].update(module=3),'typed ABI arguments differ')
reject('wrong-observed-module-width',lambda s:s['observed_argument_types'][2].update(width=4),'actual frontier formal width differs')
def stack(value):
    address=original['esp'];matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));match=matches[-1];raw=bytearray.fromhex(match[1]);raw[12:16]=value.to_bytes(4,'little');return events[:match.start(1)]+raw.hex()+events[match.end(1):]
reject('original-module-low-word',lambda s:None,'typed ABI arguments differ',stack(original['stack'][3]^1))
compare(native,stack(original['stack'][3]^0xa5a50000))
actual=[]
for name in ['module-width','omitted-formals']:
    with tempfile.TemporaryDirectory(prefix='cvpack-symbol-scopes-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='record_symbol_references')
        if name=='module-width':row['parameters'][2]['native_width']=4;reason='actual frontier formal width differs'
        else:row['parameters']=[];reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
provider=PE(ROOT/config['target']['path']);witness={0x40a9d5:'668b0f',0x40a9d8:'51',0x40f473:'668b54241c',0x40f47f:'6689501c'}
for address,hexbytes in witness.items():assert provider.read(address,len(bytes.fromhex(hexbytes))).hex()==hexbytes
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/symbol-scopes-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-symbol-scopes-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'symbol-scopes-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
    assert result.stdout==b'Unbalanced S_END record encountered\r\nnested next missing-end unmatched-end\n',(result.stdout,result.stderr)
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,actual_case_scope_links=expected,original_module_word_witness={hex(k):v for k,v in witness.items()},module_high_word_dead_positive=True,original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),unit=dict(scope='Independent native examples; not original branch coverage.',inputs=inputs,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'symbol-scopes-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
