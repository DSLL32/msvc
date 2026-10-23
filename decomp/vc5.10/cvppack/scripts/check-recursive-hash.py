#!/usr/bin/env python3
"""Recursive hash cache, scalar ABI and independently calculated native examples."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
from binary import PE
from state import BINARY,ROOT,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0081-hash-recursive-type'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state):return decoder.compare(config,data,events,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']==dict(index=4428)
table=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little');records=bytes.fromhex(native['memory'][str(table)]);record=records[332*32:333*32]
def rotate_hash(seed,values):
    for value in values:seed=((seed<<2)^(seed>>14)^value)&0xffff
    return seed&0xfff
segment=int.from_bytes(bytes.fromhex(native['globals']['line_data']),'little');offset=int.from_bytes(record[:4],'little')-segment;content=bytes.fromhex(native['memory'][str(segment)])[offset:]
assert content[2:4]==b'\x05\x10' and content[20:22]==b'\x20\0'
name=content[22:23+content[22]];expected=rotate_hash(0x1005,name[1:-1])
assert int.from_bytes(record[14:16],'little')==expected
rejected=[]
def reject(name,mutate,reason):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
def cache(state,value):
    raw=bytearray.fromhex(state['memory'][str(table)]);raw[332*32+14:332*32+16]=value.to_bytes(2,'little');state['memory'][str(table)]=raw.hex()
reject('changed-cached-hash',lambda s:cache(s,expected^1),'initialized type record fields differ')
reject('left-recursion-sentinel',lambda s:cache(s,0xfff),'initialized type record fields differ')
reject('wrong-recursive-index',lambda s:s['parameters'].update(index=4429),'typed ABI arguments differ')
reject('wrong-observed-index-width',lambda s:s['observed_argument_types'][0].update(width=2),'actual frontier formal width differs')
actual=[]
with tempfile.TemporaryDirectory(prefix='cvpack-recursive-hash-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config);row=next(row for row in changed['functions'] if row['name']=='intern_recursive_type');row['parameters'][0]['native_width']=2
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and 'actual frontier formal width differs' in diagnostic,diagnostic
    actual.append(dict(name='index-width',diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
original=PE(ROOT/config['target']['path']);dispatch=struct.unpack('<4I',original.read(0x40e80c,16));assert dispatch==(0x40e683,0x40e683,0x40e69e,0x40e6a3)
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/recursive-hash-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
data1=bytearray(range(16));data1[:4]=bytes.fromhex('0e000110')
data2=bytearray(range(48));data2[:4]=bytes.fromhex('2e000110')
ignored={j for offset in [4,12,20,28,36,44] for j in range(offset,offset+4)}
unit_expected=[rotate_hash(0x1005,b'Widge'),rotate_hash(0x1001,data1[:8]+data1[12:]),rotate_hash(0x1001,[value for i,value in enumerate(data2) if i not in ignored]),0xabc]
with tempfile.TemporaryDirectory(prefix='cvpack-recursive-hash-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'recursive-hash-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
    values=list(map(int,result.stdout.split()));assert values==unit_expected,(values,unit_expected)
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,case_cached_hash=expected,case_counted_name=name.hex(),original_dispatch=dict(start=0x40e80c,end=0x40e81c,targets=dispatch),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),unit=dict(scope='Independently calculated native named/generic-inline/generic-extended/cache examples; not original branch oracles.',values=values,inputs=inputs,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'recursive-hash-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],case_hash=expected,unit_values=values,seconds=report['seconds'])))
