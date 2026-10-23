#!/usr/bin/env python3
"""Actual reference nodes, shared flag and rewritten directory/module ownership."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0090-record-symbol-reference-directory-contract'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['reference_symbol_nodes_checked']==3 and baseline['reads_checked']==27
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
table=root('type_table_local');oldtable=oldroot('type_table_local');heads=bytes.fromhex(native['memory'][str(table)]);oldheads=original['memory'][oldtable];nodes=[]
for offset in range(0,len(heads),4):
    a=int.from_bytes(oldheads[offset:offset+4],'little');b=int.from_bytes(heads[offset:offset+4],'little')
    while a:
        left=original['memory'][a];right=bytes.fromhex(native['memory'][str(b)])
        if int.from_bytes(left[18:20],'little')==0x400:nodes.append((a,b))
        a=int.from_bytes(left[:4],'little');b=int.from_bytes(right[:4],'little')
assert len(nodes)==3
oldnode,node=nodes[0]
for name,offset,width in [('hash',4,4),('address',8,4),('name-offset',12,2),('segment',14,2),('kind',18,2),('checksum',20,4),('source-offset',24,4),('module',28,2)]:
    value=int.from_bytes(bytes.fromhex(native['memory'][str(node)])[offset:offset+width],'little')^1
    reject('changed-initialized-reference-'+name,lambda s,o=offset,v=value,w=width:word(s,node,o,v,w),'initialized reference node fields differ')
reject('both-source-offset-in-unwritten-tail',lambda s:word(s,node,24,7000),'reference source offset exceeds initialized output',original_memory(oldnode,24,7000))
reject('both-checksum-header-alias-mismatch',lambda s:word(s,node,20,0),'reference checksum/header hash alias differs',original_memory(oldnode,20,0))
reject('reference-self-chain',lambda s:word(s,node,0,node),'symbol node NULL/identity/chain differs')
changed=copy.deepcopy(native)
for a,b in nodes:word(changed,b,30,0xa55a,2)
compare(changed)
assert root('current_module_has_types')==root('statistics_enabled')==oldroot('current_module_has_types')==1
reject('lost-real-type-flag',lambda s:s['globals'].update(current_module_has_types='0000'),'application global differs')
reject('separate-historical-flag-value',lambda s:s['globals'].update(statistics_enabled='0000'),'application global differs')
info=root('current_module_info');oldinfo=oldroot('current_module_info');entry=root('directory_entries');oldentry=oldroot('directory_entries');index=root('line_entry_index')
reject('wrong-module-symbol-pointer',lambda s:word(s,info,20,root('symbol_output')),'module rewritten symbols allocation/size differs')
reject('wrong-module-symbol-size',lambda s:word(s,info,24,651),'directory/module rewritten symbol identity differs',original_memory(oldinfo,24,651))
reject('wrong-directory-symbol-pointer',lambda s:word(s,entry,index*12+4,root('symbol_output')),'directory rewritten symbol allocation alias differs')
reject('both-directory-module-size-mismatch',lambda s:word(s,entry,index*12+8,651),'directory/module rewritten symbol identity differs',original_memory(oldentry,index*12+8,651))
provider=PE(ROOT/config['target']['path']);witness={0x404763:'66391dc2544100',0x404e88:'66c705c25441000100',0x40f473:'668b54241c',0x40f47f:'6689501c',0x40f45d:'668b542424',0x40f46b:'66895012',0x40a9f5:'897718',0x40a9f8:'895f14'}
for address,hexbytes in witness.items():assert provider.read(address,len(bytes.fromhex(hexbytes))).hex()==hexbytes
actual=[]
for name in ['wrong-module-info-extent','omitted-formals']:
    with tempfile.TemporaryDirectory(prefix='cvpack-reference-frontier-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='rewrite_module_global_types')
        if name=='wrong-module-info-extent':row['parameters'][2]['native_pointee_width']=44;reason='actual pointer formal pointee extent differs'
        else:row['parameters']=[];reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/symbol-reference-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-reference-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'reference-unit';command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert not result.returncode,(result.returncode,result.stderr)
    assert result.stdout==b'initialized-fields own-padding bucket-chain\n',(result.stdout,result.stderr)
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,reference_nodes=len(nodes),own_unwritten_tail_positive=True,single_original_word_flag=0x4154c2,original_instruction_witnesses={hex(k):v for k,v in witness.items()},original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),unit=dict(scope='Independent native examples; not original branch coverage.',inputs=inputs,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'symbol-reference-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
