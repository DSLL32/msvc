#!/usr/bin/env python3
"""Initialized arena-record argument and actual index/owner alias contracts."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0084-hash-type-reference-arena-contract';native_dir=pair/'native-observer-fixed'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((native_dir/'state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in folder.glob(pattern)} for side,folder,pattern in [('original',pair/'original','events-view-*.bin'),('native',native_dir,'native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);expected=dict(arena='type_large_arena',block_index=0,offset=1604,record_indices=[302],index=4132,bytes='0a0002101c1000000a000000')
assert baseline['parameters']==dict(index=4132,data=expected) and baseline['recursive_type_nodes_checked']==9
original=decoder.decode(events)
def original_row(name):return next(row['original'] for row in data if row['name']==name)
def original_root(name):return int.from_bytes(original['memory'][original_row(name)],'little')
def native_root(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
def register(index,value):
    lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[index+1]='%08x'%value;lines[i]=' '.join(fields);return '\n'.join(lines)+'\n'
def original_word(address,offset,value):
    matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert matches
    begin,end=matches[-1].span(1);raw=bytearray.fromhex(matches[-1][1]);raw[offset:offset+4]=value.to_bytes(4,'little');return events[:begin]+raw.hex()+events[end:]
def word(state,address,offset,value):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');state['memory'][str(address)]=raw.hex()
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
for field,value in [('arena','type_arena'),('block_index',1),('offset',1605),('record_indices',[303]),('index',4131),('bytes','0a0002101d1000000a000000')]:
    reject('wrong-arena-argument-'+field,lambda s,f=field,v=value:s['parameters']['data'].update({f:v}),'typed ABI arguments differ')
reject('wrong-observed-byte-pointee',lambda s:s['observed_argument_types'][0].update(pointee_width=4),'actual frontier pointee extent differs')
header=original['memory'][original_row('type_large_arena')];block_header=original['memory'][int.from_bytes(header[20:24],'little')]
payload=int.from_bytes(block_header[4:8],'little');used=int.from_bytes(block_header[:4],'little')
reject('original-pointer-in-uninitialized-arena-tail',lambda s:None,'arena argument has no initialized owned block',register(2,payload+used))
reject('original-interior-non-record-pointer',lambda s:None,'arena argument exceeds initialized record extent',register(2,original['registers']['ecx']+1))
reject('original-wrong-index-owner',lambda s:None,'arena argument has no mapped local-record owner',register(3,4131))
old_block=int.from_bytes(original['memory'][original_row('global_type_blocks')][:4],'little');new_block=int.from_bytes(bytes.fromhex(native['globals']['global_type_blocks'])[:4],'little');slot=(4132&511)*4
reject('both-lost-global-slot-alias',lambda s:word(s,new_block,slot,0),'arena argument/global index slot alias differs',original_word(old_block,slot,0))
reject('both-lost-local-index-owner',lambda s:word(s,native_root('type_records'),302*32+4,4131),'arena argument has no mapped local-record owner',original_word(original_root('type_records'),302*32+4,4131))
actual=[]
for name in ['wrong-byte-pointee','wrong-arena-owner']:
    with tempfile.TemporaryDirectory(prefix='cvpack-arena-argument-abi-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(row for row in changed['functions'] if row['name']=='index_recursive_type');parameter=row['parameters'][0]
        if name=='wrong-byte-pointee':parameter['native_pointee_width']=4;reason='actual pointer formal pointee extent differs'
        else:parameter['arena']='type_arena';reason='arena argument has no initialized owned block'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=baseline['parameters'],recursive_nodes_checked=9,original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(native_dir/'receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),observer_sha256=sha(BINARY/'scripts/gdb-state.py'),seconds=time.perf_counter()-start)
(pair/'arena-argument-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
