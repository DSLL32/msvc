#!/usr/bin/env python3
"""Defined arena prefixes, intern-node identity and shared dispatch/index state."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import time
from state import BINARY, sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0069-copy-type-arena-corrected-alias'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']=={}
arena=struct.unpack('<6I',bytes.fromhex(native['globals']['type_large_arena']));assert arena[0:2]==(65536,1) and arena[4]==65536-40
header=arena[5];fields=struct.unpack('<3I',bytes.fromhex(native['memory'][str(header)]));assert fields[0]==40 and fields[2]==0
payload=fields[1];block=int.from_bytes(bytes.fromhex(native['globals']['global_type_blocks'])[:4],'little')
slots=struct.unpack('<2I',bytes.fromhex(native['memory'][str(block)])[:8]);assert slots==(payload,payload+24)
assert native['globals']['next_global_type']=='02100000'
assert bytes.fromhex(native['globals']['symbol_rewriters'])[304:308]==bytes.fromhex(native['globals']['next_global_type'])
positive=copy.deepcopy(native)
for address,used in [(payload,40),(block,8)]:
 raw=bytearray.fromhex(positive['memory'][str(address)]);raw[used:]=bytes([0xa5])*(len(raw)-used);positive['memory'][str(address)]=raw.hex()
compare(positive)
rejected=[]
def reject(name,mutate,reason):
 changed=copy.deepcopy(native);mutate(changed)
 try:compare(changed)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
def globalword(state,name,offset,value):
 raw=bytearray.fromhex(state['globals'][name]);raw[offset:offset+4]=value.to_bytes(4,'little');state['globals'][name]=raw.hex()
def word(state,address,offset,value):
 raw=bytearray.fromhex(state['memory'][str(address)]);raw[offset:offset+4]=value.to_bytes(4,'little');state['memory'][str(address)]=raw.hex()
reject('wrong-arena-capacity',lambda s:globalword(s,'type_large_arena',0,32768),'arena capacity/count/remaining differs')
reject('wrong-block-count',lambda s:globalword(s,'type_large_arena',4,2),'arena capacity/count/remaining differs')
reject('wrong-last-block-alias',lambda s:globalword(s,'type_large_arena',8,header+4),'arena last-block alias differs')
reject('wrong-cursor-offset',lambda s:globalword(s,'type_large_arena',12,payload+39),'arena cursor alias/offset differs')
reject('wrong-remaining-count',lambda s:globalword(s,'type_large_arena',16,65536-39),'arena capacity/count/remaining differs')
reject('wrong-header-allocation-owner',lambda s:globalword(s,'type_large_arena',20,header+4),'arena block header allocation differs')
reject('wrong-initialized-prefix-size',lambda s:word(s,header,0,39),'arena initialized extent differs')
reject('wrong-payload-allocation-owner',lambda s:word(s,header,4,payload+4),'arena payload allocation differs')
reject('missing-next-link',lambda s:word(s,header,8,header),'arena chain NULL/identity/cycle differs')
reject('changed-defined-arena-byte',lambda s:word(s,payload,8,0),'initialized arena payload bytes differ')
reject('incomplete-arena-capacity',lambda s:s['memory'].update({str(payload):s['memory'][str(payload)][:-2]}),'incomplete arena payload capacity')
reject('wrong-global-type-pointer-slot',lambda s:word(s,block,0,payload+1),'type payload arena/offset alias differs')
reject('incomplete-global-pointer-block',lambda s:s['memory'].update({str(block):s['memory'][str(block)][:-2]}),'incomplete global type pointer-block capacity')
record_root=int.from_bytes(bytes.fromhex(native['globals']['type_records']),'little')
reject('wrong-packed-local-record-alias',lambda s:word(s,record_root,12*32,payload+1),'type payload arena/offset alias differs')
node=native['checked_allocations'][-2]['address'];assert int.from_bytes(bytes.fromhex(native['memory'][str(node)])[:4],'little')==0x1000
reject('wrong-interned-index',lambda s:word(s,node,0,0x1001),'interned-type index/extent differs')
reject('wrong-interned-payload-alias',lambda s:word(s,node,4,payload+1),'type payload arena/offset alias differs')
reject('lost-dispatch-index-storage-alias',lambda s:globalword(s,'symbol_rewriters',304,0x1000),'symbol callback kind/reserved differs')
# Independent actual ELF DWARF layout: the graph metadata cannot invent a
# separate global or shift the shared index away from the original DWORD.
command=['gdb','-q','-nx','-batch',str(BINARY/'build/native/CVPACK'),'-ex',
 'python import gdb,json; t=gdb.lookup_type("TypeArena"); b=gdb.lookup_type("TypeArenaBlock"); v=gdb.parse_and_eval("symbol_rewriters.state.next_global_type"); root=gdb.parse_and_eval("symbol_rewriters"); print("LAYOUT "+json.dumps([int(t.sizeof),int(b.sizeof),int(v.address)-int(root.address),int(v.type.sizeof)]))']
out=subprocess.check_output(command,text=True,stderr=subprocess.STDOUT);actual=json.loads(next(line[7:] for line in out.splitlines() if line.startswith('LAYOUT ')))
assert actual==[24,12,304,4]
audit=json.loads((pair/'original/audit.json').read_text());alias=audit['data_aliases'];assert len(alias)==1 and alias[0]['actual_offset']==304 and alias[0]['actual_size']==4
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,actual_dwarf_layout=actual,actual_data_alias_audit=alias,
 native_own_arena_and_pointer_block_tails_positive=True,defined_arena_bytes=40,defined_global_pointer_slots=2,interned_nodes=2,
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'type-arena-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
