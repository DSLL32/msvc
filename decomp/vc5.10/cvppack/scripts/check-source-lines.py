#!/usr/bin/env python3
"""Source-line ownership and actual call-time rewrite provenance at module rollover."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0091-copy-module-source-lines';native_dir=pair/'native-call-time-operands'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((native_dir/'state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={'original':{p.name:p.read_bytes() for p in (pair/'original').glob('events-view-*.bin')},'native':{p.name:p.read_bytes() for p in native_dir.glob('native-view-*.bin')}}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['reference_symbol_nodes_checked']==3 and baseline['reads_checked']==29
original=decoder.decode(events)
def root(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
def oldroot(name):return int.from_bytes(original['memory'][next(r['original'] for r in data if r['name']==name)],'little')
def word(state,address,offset,value,width=4):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+width]=value.to_bytes(width,'little');state['memory'][str(address)]=raw.hex()
def original_memory(address,offset,value,width=4):
    matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert matches
    begin,end=matches[-1].span(1);raw=bytearray.fromhex(matches[-1][1]);raw[offset:offset+width]=value.to_bytes(width,'little');return events[:begin]+raw.hex()+events[end:]
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
entry=root('directory_entries');oldentry=oldroot('directory_entries');rows=bytes.fromhex(native['memory'][str(entry)])
indexes=[i//12 for i in range(0,len(rows),12) if rows[i:i+4]==bytes.fromhex('27010200')];assert indexes==[6]
offset=6*12+4;lines=int.from_bytes(rows[offset:offset+4],'little');oldlines=int.from_bytes(original['memory'][oldentry][offset:offset+4],'little')
modules=bytes.fromhex(native['memory'][str(root('module_index'))]);oldmodules=original['memory'][oldroot('module_index')]
indexes=[i for i in range(0,len(modules),12) if modules[i:i+2]==b'\x02\x00'];assert len(indexes)==1
info=int.from_bytes(modules[indexes[0]+8:indexes[0]+12],'little');oldinfo=int.from_bytes(oldmodules[indexes[0]+8:indexes[0]+12],'little')
assert info!=root('current_module_info') and int.from_bytes(bytes.fromhex(native['memory'][str(info)])[28:32],'little')==lines
assert len(bytes.fromhex(native['memory'][str(lines)]))==256
reject('changed-source-line-byte',lambda s:word(s,lines,31,0xff,1),'directory source lines contents differ')
reject('wrong-directory-copy-pointer',lambda s:word(s,entry,offset,root('module_data')),'directory source lines allocation alias differs')
reject('wrong-module-copy-pointer',lambda s:word(s,info,28,root('module_data')),'module source lines allocation alias differs')
reject('wrong-module-copy-size',lambda s:word(s,info,32,255),'target object bytes differ')
reject('truncated-copy',lambda s:s['memory'].update({str(lines):s['memory'][str(lines)][:-2]}),'directory source lines extent differs')
allocation=next(r for r in native['image_allocations'] if r.get('owner')=='rewrite_module_symbols')
assert allocation['source']==dict(entry_index=4,module=2,input_size=8093,symbols=612,references=0)
for name in ['entry_index','module','input_size','symbols','references']:
    reject('wrong-call-time-'+name,lambda s,n=name:next(r for r in s['image_allocations'] if r.get('owner')=='rewrite_module_symbols')['source'].update({n:allocation['source'][n]+1}),'symbol rewrite actual call-time operands differ')
reject('missing-call-time-operands-after-rollover',lambda s:next(r for r in s['image_allocations'] if r.get('owner')=='rewrite_module_symbols').pop('source'),'historical symbol rewrite proof ends before module rollover')
# This is the actual watched WORD before allocation, not the reset final global.
row=next(r for r in data if r['name']=='module_symbol_count');begin=events.index('image-allocation-entry 0040a7d1 ');end=events.index('image-allocation-call 0040a7d1 ',begin)
match=re.search(r'^memory %08x ([0-9a-f]+)$'%row['original'],events[begin:end],re.M);assert match and match[1]=='6402'
a,b=begin+match.start(1),begin+match.end(1);bad=events[:a]+'6502'+events[b:]
reject('changed-original-call-time-word',lambda s:None,'symbol rewrite allocation expression differs',bad)
reject('wrong-object-output-width',lambda s:s['parameters']['object'].update(width=1),'typed ABI arguments differ')
provider=PE(ROOT/config['target']['path']);witness={0x4054b9:'895d04',0x4054bd:'89581c',0x4054c1:'894820',0x4054c5:'c3'}
for address,hexbytes in witness.items():assert provider.read(address,len(bytes.fromhex(hexbytes))).hex()==hexbytes
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/source-lines-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-source-lines-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'source-lines-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
    assert result.stdout==b'source-lines copied-all-bytes directory-module-alias\n',(result.stdout,result.stderr)
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,source_lines_bytes=256,source_line_original_module=2,current_module=3,rewrite_call_time_operands=allocation['source'],original_instruction_witnesses={hex(k):v for k,v in witness.items()},original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(native_dir/'receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),unit=dict(scope='Independent native example; not original branch coverage.',inputs=inputs,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'source-lines-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
