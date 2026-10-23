#!/usr/bin/env python3
"""Global/public bucket ownership, initialized nodes and duplicate/conflict checks."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY, ROOT, sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0078-insert-global-symbol'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']==dict(record=dict(root='type_records',index=303),offset=8)
def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
table=root(native,'type_table_local');heads=bytes.fromhex(native['memory'][str(table)])
nodes=[(i,int.from_bytes(heads[i:i+4],'little')) for i in range(0,len(heads),4) if any(heads[i:i+4])]
assert len(nodes)==1 and nodes[0][0]//4==1764
slot,node=nodes[0];assert root(native,'global_symbol_count')==1 and root(native,'global_symbol_bytes')==28
payload=bytes.fromhex(native['memory'][str(node)]);assert len(payload)==44 and payload[16:]==bytes.fromhex('1a00031014100000107461675250434f4c454d455353414745000000')
original=decoder.decode(events)
def original_root(name):return int.from_bytes(original['memory'][next(row['original'] for row in data if row['name']==name)],'little')
original_node=int.from_bytes(original['memory'][original_root('type_table_local')][slot:slot+4],'little')
def original_word(address,offset,value):
    matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert matches
    begin,end=matches[-1].span(1);raw=bytearray.fromhex(matches[-1][1]);raw[offset:offset+4]=value.to_bytes(4,'little')
    return events[:begin]+raw.hex()+events[end:]
def word(state,address,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');state['memory'][str(address)]=raw.hex()
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,offset in [('hash',4),('offset',8),('name-and-segment',12),('record',16),('type-index',20),('name',28),('padding',40)]:
    reject('changed-global-'+field,lambda s,o=offset:word(s,node,o,0xffffffff),'initialized public node header/record/padding differs')
reject('lost-global-bucket-node',lambda s:word(s,table,slot,0),'symbol node NULL/identity/chain differs')
reject('wrong-global-node-arena-offset',lambda s:word(s,table,slot,node+4),'public node arena/offset differs')
reject('truncated-global-buckets',lambda s:s['memory'].update({str(table):s['memory'][str(table)][:-8]}),'incomplete symbol bucket observation')
reject('truncated-global-node',lambda s:s['memory'].update({str(node):s['memory'][str(node)][:-2]}),'incomplete or unowned initialized symbol node extent')
hash_value=int.from_bytes(payload[4:8],'little')^1
reject('both-wrong-hash-bucket',lambda s:word(s,node,4,hash_value),'symbol node hash/bucket differs',original_word(original_node,4,hash_value))
reject('both-cyclic-global-chain',lambda s:word(s,node,0,node),'symbol node NULL/identity/chain differs',original_word(original_node,0,original_node))
for name in ['global_symbol_count','global_symbol_bytes','duplicate_public_symbols']:
    reject('changed-'+name,lambda s,n=name:s['globals'].update({n:(root(s,n)^1).to_bytes(4,'little').hex()}),'application global differs')
reject('wrong-reference-offset',lambda s:s['parameters'].update(offset=9),'typed ABI arguments differ')
reject('wrong-reference-record',lambda s:s['parameters']['record'].update(index=304),'typed ABI arguments differ')
reject('failed-original-symbol-observation',lambda s:None,'incomplete initialized allocation observation',events+'public-node-observation-failed\n')
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/global-symbol-unit.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-global-symbol-unit-',dir=Path.home()/'.ai-tmp') as temporary:
    executable=Path(temporary)/'global-symbol-unit'
    command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
    before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
    assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
    result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert result.returncode==0,(result.returncode,result.stderr)
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,parameters=baseline['parameters'],global_nodes=1,global_record_bytes=28,original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),unit=dict(scope='Native focused new/duplicate/conflict branches, not an original oracle.',inputs=inputs,status=result.returncode,command=command,build_seconds=build_seconds),seconds=time.perf_counter()-start)
(pair/'global-symbol-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],unit_status=result.returncode,seconds=report['seconds'])))
