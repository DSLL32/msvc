#!/usr/bin/env python3
"""Actual recursive bucket nodes, pool units and low-word ordinal contracts."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0083-mark-recursive-type'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['recursive_type_nodes_checked']==1
assert baseline['parameters']==dict(record=dict(root='type_records',index=331),ordinal=2)
def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
table=root(native,'type_table_primary');heads=bytes.fromhex(native['memory'][str(table)])
nodes=[(i,int.from_bytes(heads[i:i+4],'little')) for i in range(0,len(heads),4) if any(heads[i:i+4])]
assert len(nodes)==1 and nodes[0][0]//4==174
slot,node=nodes[0];assert root(native,'recursive_node_cursor')==node+12 and root(native,'recursive_nodes_remaining')==1023
original=decoder.decode(events)
def original_root(name):return int.from_bytes(original['memory'][next(row['original'] for row in data if row['name']==name)],'little')
original_node=int.from_bytes(original['memory'][original_root('type_table_primary')][slot:slot+4],'little')
def original_memory(address,offset,raw):
    matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert matches
    begin,end=matches[-1].span(1);payload=bytearray.fromhex(matches[-1][1]);payload[offset:offset+len(raw)]=raw
    return events[:begin]+payload.hex()+events[end:]
def register(index,value):
    lines=events.splitlines();i=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[i].split();fields[index+1]='%08x'%value;lines[i]=' '.join(fields);return '\n'.join(lines)+'\n'
def word(state,address,offset,value,width=4):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+width]=value.to_bytes(width,'little');state['memory'][str(address)]=raw.hex()
rejected=[]
def reject(name,mutate,reason,raw=events):
    state=copy.deepcopy(native);mutate(state)
    try:compare(state,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('lost-recursive-node',lambda s:word(s,table,slot,0),'recursive-type node NULL/identity/chain differs')
reject('wrong-node-pool-offset',lambda s:word(s,table,slot,node+12),'recursive-type pool/offset alias differs')
reject('wrong-node-payload-alias',lambda s:word(s,node,0,int.from_bytes(bytes.fromhex(s['memory'][str(node)])[:4],'little')+1),'type payload arena/offset alias differs')
reject('wrong-node-global-index',lambda s:word(s,node,4,0x1000),'recursive-type index differs')
reject('truncated-node',lambda s:s['memory'].update({str(node):s['memory'][str(node)][:-2]}),'incomplete initialized recursive-type node')
reject('truncated-head-table',lambda s:s['memory'].update({str(table):s['memory'][str(table)][:-8]}),'incomplete recursive-type bucket observation')
reject('both-cyclic-node',lambda s:word(s,node,8,node),'recursive-type node NULL/identity/chain differs',original_memory(original_node,8,original_node.to_bytes(4,'little')))
record_table=root(native,'type_records');original_table=original_root('type_records')
reject('both-wrong-record-bucket',lambda s:word(s,record_table,332*32+14,175,2),'recursive-type record/hash bucket differs',original_memory(original_table,332*32+14,(175).to_bytes(2,'little')))
counts=root(native,'type_table_primary_counts');original_counts=original_root('type_table_primary_counts')
reject('both-wrong-chain-count',lambda s:word(s,counts,slot//2,0,2),'recursive-type bucket count differs',original_memory(original_counts,slot//2,b'\0\0'))
reject('wrong-pool-cursor',lambda s:s['globals'].update(recursive_node_cursor=(node+24).to_bytes(4,'little').hex()),'arena cursor/capacity/offset differs')
remaining_row=next(row for row in data if row['name']=='recursive_nodes_remaining')
reject('both-wrong-pool-unit-count',lambda s:s['globals'].update(recursive_nodes_remaining=(1022).to_bytes(4,'little').hex()),'arena cursor/capacity/offset differs',original_memory(remaining_row['original'],0,(1022).to_bytes(4,'little')))
reject('wrong-ordinal',lambda s:s['parameters'].update(ordinal=3),'typed ABI arguments differ')
reject('original-wrong-ordinal-low-word',lambda s:None,'typed ABI arguments differ',register(3,original['registers']['edx']^1))
compare(native,register(3,original['registers']['edx']^0x12340000))
provider=PE(ROOT/config['target']['path']);assert provider.read(0x40e825,5)==bytes.fromhex('6689542400')
actual=[]
with tempfile.TemporaryDirectory(prefix='cvpack-recursive-node-abi-',dir=Path.home()/'.ai-tmp') as temporary:
    directory=Path(temporary);changed=copy.deepcopy(config);row=next(row for row in changed['functions'] if row['name']=='hash_type_reference');row['parameters'][1]['native_width']=4
    bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
    result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
    diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and 'actual frontier formal width differs' in diagnostic,diagnostic
    actual.append(dict(name='ordinal-width',diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
expression='python import gdb,json; print("NODE-LAYOUT "+json.dumps([int(gdb.parse_and_eval(x)) for x in ["sizeof(RecursiveTypeNode)","(unsigned)&((RecursiveTypeNode*)0)->data","(unsigned)&((RecursiveTypeNode*)0)->index","(unsigned)&((RecursiveTypeNode*)0)->next"]]))'
layout_output=subprocess.check_output(['gdb','-q','-nx','-batch',str(BINARY/'build/native/CVPACK'),'-ex',expression],text=True,stderr=subprocess.STDOUT)
layout=json.loads(next(line[12:] for line in layout_output.splitlines() if line.startswith('NODE-LAYOUT ')))
assert layout==[12,0,4,8],'actual recursive node DWARF layout differs'
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=baseline['parameters'],actual_dwarf_layout=layout,recursive_nodes=1,node_pool_bytes=12288,node_size=12,remaining=1023,bucket=174,dead_ordinal_high_word_positive=True,original_word_consumption=dict(address=0x40e825,bytes='6689542400'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'recursive-node-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
