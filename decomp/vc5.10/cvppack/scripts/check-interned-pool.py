#!/usr/bin/env python3
"""Recursive interned pool nodes and DWORD caller-local output ABI controls."""
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

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0085-index-recursive-type-dword-output'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['interned_type_nodes_checked']==76 and baseline['pooled_interned_nodes_checked']==32
assert baseline['parameters']==dict(index=203,from_global=dict(owner='compact_type_index',local='from_global',width=4))
original=decoder.decode(events)
def root(state,name):return int.from_bytes(bytes.fromhex(state['globals'][name]),'little')
def oldroot(name):return int.from_bytes(original['memory'][next(r['original'] for r in data if r['name']==name)],'little')
def word(state,address,offset,value,width=4):
    raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+width]=value.to_bytes(width,'little');state['memory'][str(address)]=raw.hex()
def original_memory(address,offset,value,width=4):
    matches=list(re.finditer(r'^memory %08x ([0-9a-f]+)$'%address,events,re.M));assert matches
    begin,end=matches[-1].span(1);raw=bytearray.fromhex(matches[-1][1]);raw[offset:offset+width]=value.to_bytes(width,'little');return events[:begin]+raw.hex()+events[end:]
def stack(slot,value):return original_memory(original['esp'],slot*4,value)

table=root(native,'type_table_secondary');oldtable=oldroot('type_table_secondary');counts=root(native,'type_table_secondary_counts');oldcounts=oldroot('type_table_secondary_counts')
heads=bytes.fromhex(native['memory'][str(table)]);cursor=root(native,'interned_node_cursor');remaining=root(native,'interned_nodes_remaining');assert remaining==992
pool=cursor-32*12
pooled=[(offset,int.from_bytes(heads[offset:offset+4],'little')) for offset in range(0,len(heads),4) if pool<=int.from_bytes(heads[offset:offset+4],'little')<cursor]
assert pooled;offset,node=pooled[0];oldnode=int.from_bytes(original['memory'][oldtable][offset:offset+4],'little')
rejected=[]
def reject(name,mutate,reason,raw=events):
    changed=copy.deepcopy(native);mutate(changed)
    try:compare(changed,raw)
    except (AssertionError,KeyError) as error:
        assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
    else:raise AssertionError('accepted '+name)
reject('lost-pooled-node',lambda s:word(s,table,offset,0),'interned-type node NULL/identity/chain differs')
reject('wrong-pool-node-offset',lambda s:word(s,table,offset,node+12),'interned-type pool/offset alias differs')
oldcursor=oldroot('interned_node_cursor')
reject('both-node-in-unused-pool-tail',lambda s:word(s,table,offset,cursor),'interned-type node exceeds initialized pool prefix',original_memory(oldtable,offset,oldcursor))
reject('truncated-initialized-node',lambda s:s['memory'].update({str(node):s['memory'][str(node)][:-2]}),'interned-type index/extent differs')
reject('wrong-global-index',lambda s:word(s,node,0,0x1000),'interned-type index/extent differs')
reject('wrong-payload-alias',lambda s:word(s,node,4,int.from_bytes(bytes.fromhex(s['memory'][str(node)])[4:8],'little')+1),'type payload arena/offset alias differs')
reject('both-wrong-chain-count',lambda s:word(s,counts,offset//2,0,2),'interned-type bucket count differs',original_memory(oldcounts,offset//2,0,2))
reject('wrong-pool-cursor',lambda s:s['globals'].update(interned_node_cursor=(cursor+12).to_bytes(4,'little').hex()),'arena cursor/capacity/offset differs')
remaining_row=next(r for r in data if r['name']=='interned_nodes_remaining')
reject('both-wrong-pool-unit-count',lambda s:s['globals'].update(interned_nodes_remaining=(remaining-1).to_bytes(4,'little').hex()),'interned-type active pool/cursor ownership differs',original_memory(remaining_row['original'],0,remaining-1))
reject('wrong-output-width',lambda s:s['parameters']['from_global'].update(width=1),'typed ABI arguments differ')
reject('wrong-output-owner',lambda s:s['parameters']['from_global'].update(owner='compact_legacy_index'),'typed ABI arguments differ')
reject('wrong-actual-pointee',lambda s:s['observed_argument_types'][1].update(pointee_width=1),'actual frontier pointee extent differs')
reject('original-interior-output-pointer',lambda s:None,'original output argument caller-local identity differs',stack(2,original['stack'][2]+1))
# No output bytes are initialized before this stub. Each side owns its current local bytes.
native_output=int(re.search(r'from_global=(0x[0-9a-f]+)',(pair/'native/gdb-stdout').read_text())[1],16)
changed=copy.deepcopy(native);changed['memory'][str(native_output)]='ffffffff';compare(changed,events+'memory %08x 12345678\n'%original['stack'][2])
actual=[]
for name in ['byte-width','wrong-owner','omitted-formals']:
    with tempfile.TemporaryDirectory(prefix='cvpack-dword-output-',dir=Path.home()/'.ai-tmp') as temporary:
        directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='lookup_global_type')
        if name=='byte-width':row['parameters'][1]['width']=1;reason='actual pointer formal pointee extent differs'
        elif name=='wrong-owner':row['parameters'][1]['owner']='compact_legacy_index';reason='AssertionError'
        else:row['parameters']=[];reason='incomplete frontier ABI contract'
        bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
        result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
        diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
        actual.append(dict(name=name,diagnostic=diagnostic,observer_sha256=sha(output/'gdb-state.py'),bad_config_sha256=sha(bad)))
provider=PE(ROOT/config['target']['path'])
witness={0x402c25:'c70200000000',0x402c71:'8902',0x403139:'8d4c2418',0x40e820:'51',0x40e825:'6689542400',0x40e89b:'8d4c2400',0x40e8a7:'e87443ffff',0x40e8af:'f6401c04',0x40e8b5:'668b400e',0x40e8bb:'8bc8',0x40e8bd:'e87efdffff'}
for address,hexbytes in witness.items():assert provider.read(address,len(bytes.fromhex(hexbytes))).hex()==hexbytes
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,pooled_nodes=32,total_interned_nodes=76,pool_size=12288,node_size=12,remaining=remaining,parameters=baseline['parameters'],uninitialized_output_prebytes_not_compared=True,original_instruction_witnesses={hex(address):value for address,value in witness.items()},union_liveness='40e825 stores WORD ordinal in DWORD PUSH ECX local; only offset selection reads ordinal before40e89b passes that exact slot; after lookup40e8af only returned record status/hash is consumed. Original and native pop/discard local without a later ordinal read.',corrected_caller_sources={str(p.relative_to(BINARY)):sha(p) for p in [BINARY/'src'/name for name in ['types.h','stubs.c','modern-types.c','legacy-types.c','type-lists.c','symbol-scan.c','type-recursive.c','type-recursive-hash.c']]},original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'interned-pool-output-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
