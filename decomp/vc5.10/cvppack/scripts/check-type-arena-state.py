#!/usr/bin/env python3
"""Defined arena prefixes, intern-node identity and shared dispatch/index state."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import time
from state import BINARY, sha
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--pair',type=Path,default=BINARY/'cases/hello-debug-dll/evidence/0069-copy-type-arena-corrected-alias');args=parser.parse_args()
start=time.perf_counter();pair=args.pair.resolve()
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()}
views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native)
if pair.name.startswith('0074-'):expected_used,expected_slots=328,7
elif pair.name.startswith('0073-'):expected_used,expected_slots=144,4
else:expected_used,expected_slots=40,2
arena=struct.unpack('<6I',bytes.fromhex(native['globals']['type_large_arena']));assert arena[0:2]==(65536,1) and arena[4]==65536-expected_used
header=arena[5];fields=struct.unpack('<3I',bytes.fromhex(native['memory'][str(header)]));assert fields[0]==expected_used and fields[2]==0
payload=fields[1];block=int.from_bytes(bytes.fromhex(native['globals']['global_type_blocks'])[:4],'little')
slots=struct.unpack('<2I',bytes.fromhex(native['memory'][str(block)])[:8]);assert slots==(payload,payload+24)
assert int.from_bytes(bytes.fromhex(native['globals']['next_global_type']),'little')==0x1000+expected_slots
assert bytes.fromhex(native['globals']['symbol_rewriters'])[304:308]==bytes.fromhex(native['globals']['next_global_type'])
positive=copy.deepcopy(native)
for address,used in [(payload,expected_used),(block,expected_slots*4)]:
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
node=next(row['address'] for row in native['checked_allocations'] if row['size']==12 and str(row['address']) in native['memory'] and int.from_bytes(bytes.fromhex(native['memory'][str(row['address'])])[:4],'little')==0x1000)
reject('wrong-interned-index',lambda s:word(s,node,0,0x1001),'interned-type index/extent differs')
reject('wrong-interned-payload-alias',lambda s:word(s,node,4,payload+1),'type payload arena/offset alias differs')
reject('lost-dispatch-index-storage-alias',lambda s:globalword(s,'symbol_rewriters',304,0x1000),'symbol callback kind/reserved differs')
if expected_slots>=4:
 root=int.from_bytes(bytes.fromhex(native['globals']['type_positions']),'little');heads=bytes.fromhex(native['memory'][str(root)])
 populated=[int.from_bytes(heads[offset:offset+4],'little') for offset in range(0,2048,4) if any(heads[offset:offset+4])];assert len(populated)==(2 if expected_slots==7 else 1)
 cached=populated[0];cache=struct.unpack('<5I',bytes.fromhex(native['memory'][str(cached)]));assert cache[2:]==(0x1003,0,0)
 assert bytes.fromhex(native['memory'][str(cache[0])])==b'\x07tagRECT'
 def wrong_name_owner(state):
  word(state,cached,0,cache[0]+1)
  state['memory'][str(cache[0]+1)]=state['memory'][str(cache[0])]
 reject('wrong-global-cache-counted-name-owner',wrong_name_owner,'type payload arena/offset alias differs')
 reject('wrong-global-cache-record-alias',lambda s:word(s,cached,4,cache[1]+1),'type payload arena/offset alias differs')
 reject('wrong-global-cache-index',lambda s:word(s,cached,8,0x1002),'type-name index/reserved fields differ')
 reject('changed-global-borrowed-counted-name',lambda s:s['memory'].update({str(cache[0]):b'\x07tagRECU'.hex()}),'type-name counted string bytes differ')
 if native['function']=='record_base_classes':
  for name,value in [('index',0x1002),('list',0x1003)]:
   reject('wrong-base-class-'+name,lambda s,n=name,v=value:s['parameters'].update({n:v}),'typed ABI arguments differ')
 else:
  reject('lost-public-scope-depth',lambda s:globalword(s,'public_scope_depth',0,2),'application global differs')
  reject('wrong-discard-scope-depth',lambda s:globalword(s,'discard_symbol_depth',0,1),'application global differs')
# Independent actual ELF DWARF layout: the graph metadata cannot invent a
# separate global or shift the shared index away from the original DWORD.
command=['gdb','-q','-nx','-batch',str(BINARY/'build/native/CVPACK'),'-ex',
 'python import gdb,json; t=gdb.lookup_type("TypeArena"); b=gdb.lookup_type("TypeArenaBlock"); v=gdb.parse_and_eval("symbol_rewriters.state.next_global_type"); root=gdb.parse_and_eval("symbol_rewriters"); print("LAYOUT "+json.dumps([int(t.sizeof),int(b.sizeof),int(v.address)-int(root.address),int(v.type.sizeof)]))']
out=subprocess.check_output(command,text=True,stderr=subprocess.STDOUT);actual=json.loads(next(line[7:] for line in out.splitlines() if line.startswith('LAYOUT ')))
assert actual==[24,12,304,4]
audit=json.loads((pair/'original/audit.json').read_text());alias=audit['data_aliases'];assert len(alias)==1 and alias[0]['actual_offset']==304 and alias[0]['actual_size']==4
report=dict(schema=1,native_only=True,count=len(rejected),rejected=rejected,actual_dwarf_layout=actual,actual_layout_elf_sha256=sha(BINARY/'build/native/CVPACK'),actual_data_alias_audit=alias,
 native_own_arena_and_pointer_block_tails_positive=True,defined_arena_bytes=expected_used,defined_global_pointer_slots=expected_slots,interned_nodes=expected_slots,
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'type-arena-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
