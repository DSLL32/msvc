#!/usr/bin/env python3
"""Complete compared ranges/results and initialized symbol-allocation arguments."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0087-compare-recursive-type-initialized-operands';native_dir=pair/'native-plt-finish'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((native_dir/'state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in folder.glob(pattern)} for side,folder,pattern in [('original',pair/'original','events-view-*.bin'),('native',native_dir,'native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['type_compare_calls_checked']==2
assert [row['size'] for row in native['type_compare_calls']]==[8,28]
assert [row['different'] for row in native['type_compare_calls']]==[False,True]
assert baseline['parameters']['size']==652 and baseline['parameters']['buffer']['capacity']==8705
original=decoder.decode(events);rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('lost-comparison',lambda s:s['type_compare_calls'].pop(),'recursive comparison call inventory differs')
reject('changed-compared-byte',lambda s:s['type_compare_calls'][1].update(right='ff'+s['type_compare_calls'][1]['right'][2:]),'recursive comparison operands/zero-result differ')
reject('changed-comparison-length',lambda s:s['type_compare_calls'][0].update(size=9),'recursive comparison operands/zero-result differ')
reject('changed-comparison-result',lambda s:s['type_compare_calls'][0].update(different=True),'recursive comparison operands/zero-result differ')
reject('both-wrong-result',lambda s:s['type_compare_calls'][0].update(different=True),'recursive comparison result contradicts complete inputs',events.replace('type-compare-return 0\n','type-compare-return 1\n',1))
match=re.search(r'^type-compare-call (.+)$',events,re.M);fields=match[1].split();fields[3]=fields[3][:-2];truncated=events[:match.start(1)]+' '.join(fields)+events[match.end(1):]
reject('truncated-original-range',lambda s:None,'truncated recursive comparison bytes',truncated)
for field,value in [('offset',1),('capacity',8704),('size',653),('owner','scan_module_symbols'),('bytes','ff'+native['parameters']['buffer']['bytes'][2:])]:
    reject('changed-initialized-output-'+field,lambda s,f=field,v=value:s['parameters']['buffer'].update({f:v}),'typed ABI arguments differ')
# Each side's allocation tail is not initialized by this prefix.
owner=next(row for row in native['image_allocations'] if row.get('owner')=='rewrite_module_symbols');base=owner['address'];changed=copy.deepcopy(native);raw=bytearray.fromhex(changed['memory'][str(base)]);assert len(raw)==8705;raw[652:] = b'\xa5'*(8705-652);changed['memory'][str(base)]=raw.hex();compare(changed)
actual=[]
with tempfile.TemporaryDirectory(prefix='cvpack-symbol-buffer-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='link_symbol_scopes');row['parameters'][0]['base_argument']=False
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and 'symbol output argument ownership/extent differs' in diagnostic,diagnostic
    actual.append(dict(name='base-versus-cursor',diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
provider=PE(ROOT/config['target']['path']);assert provider.read(0x408751,2)==b'\x85\xc0';assert provider.read(0x410326,6)==bytes.fromhex('ff25fc614100')
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/recursive-compare-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-recursive-compare-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'recursive-compare-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
    assert result.stdout==b'equal different recursive conflicting-alias\n'
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,observed_comparisons=native['type_compare_calls'],consumed_return='408751 TEST EAX,EAX; only zero/nonzero is compared',parameters={k:v for k,v in baseline['parameters'].items() if k!='buffer'},buffer={k:v for k,v in baseline['parameters']['buffer'].items() if k!='bytes'},own_unwritten_tail_positive_bytes=8705-652,original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(native_dir/'receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),unit=dict(scope='Independent native examples; not original branch coverage.',inputs=inputs,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'recursive-compare-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
