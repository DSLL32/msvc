#!/usr/bin/env python3
"""Finite post-sort allocation/write consumer witness, without CFG discovery."""
import importlib.util
import json
import hashlib
import struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_REG_ESP,X86_REG_EDX
from binary import PE
from state import BINARY,ROOT,sha

pair=BINARY/'cases/hello-debug-dll/evidence/0104-output-buffer-initialization'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
spec=importlib.util.spec_from_file_location('buffer_evidence_raw',BINARY/'scripts/raw-state.py');raw=importlib.util.module_from_spec(spec);spec.loader.exec_module(raw)
original=raw.decode((pair/'original/events').read_text());native=json.loads((pair/'native/state.json').read_text())
path=ROOT/config['target']['path'];assert sha(path)==config['target']['sha256'];pe=PE(path);decoder=Cs(CS_ARCH_X86,CS_MODE_32);decoder.detail=True
rows=[]
for name,start,end,allowed,expected_calls in [
 ('initializer',0x4100e0,0x410135,{0x412d34,0x4161bc,0x412d38,0x412d40,0x412d3c},[(0x4100f3,'esi')]),
 ('writer_after_initializer',0x406b84,0x406c17,{0x412d34,0x412d38,0x412d40,0x412138,0x41213c,0x412124,0x415640,0x415660},[(0x406bcc,'0x410140'),(0x406be2,'0x4054d0'),(0x406bf0,'0x401a80'),(0x406c12,'0x4068a0')]),
 ('tell_cached_file',0x401a80,0x401a9e,{0x41565c},[])]:
 body=pe.read(start,end-start);instructions=list(decoder.disasm(body,start));assert sum(i.size for i in instructions)==len(body)
 assert not any(i.mnemonic.startswith(('f','v','xsave','xrstor')) for i in instructions)
 calls=[(i.address,i.op_str) for i in instructions if i.mnemonic=='call'];assert calls==expected_calls
 absolute=[];writes=[]
 for instruction in instructions:
  for index,operand in enumerate(instruction.operands):
   if operand.type!=X86_OP_MEM:continue
   memory=operand.mem
   if not memory.base and not memory.index:
    assert memory.disp in allowed,'new finite path consumes a retired root'
    absolute.append(dict(site=instruction.address,address=memory.disp,width=operand.size))
   elif name=='initializer':assert memory.base==X86_REG_ESP and not memory.index and memory.disp==8,'unproved initializer non-global operand'
   if name=='writer_after_initializer' and instruction.mnemonic=='mov' and index==0:
    if memory.base:
     assert memory.base==X86_REG_EDX and not memory.index and memory.disp in (0,4) and operand.size==4,'unknown writer payload destination'
     writes.append(dict(site=instruction.address,offset=memory.disp,width=operand.size))
 if name=='writer_after_initializer':assert writes==[dict(site=0x406ba6,offset=0,width=4),dict(site=0x406bae,offset=4,width=4)]
 rows.append(dict(name=name,extent=[start,end],sha256=hashlib.sha256(body).hexdigest(),absolute_operands=absolute,calls=[dict(site=a,target=b) for a,b in calls],new_payload_writes=writes))
def root(name):
 row=next(r for r in data if r['name']==name);return int.from_bytes(original['memory'][row['original']],'little')
def actual(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
recorded=[]
for side,state,glob in [('original',original,root),('native',native,actual)]:
 new=state['output_buffer_allocations'][0];old=state['main_free_calls'][11]
 assert glob('type_records')==old['address'] and glob('output_buffer')==new['address']
 recorded.append(dict(side=side,retired_type_records_address=old['address'],retired_size=old['size'],new_output_buffer_address=new['address'],new_size=new['size'],actual_dangling_root_aliases_new_owner=old['address']==new['address'],old_payload_role='immutable pre-free historical type records only',new_payload_role='separate malloc generation:8 initialized bytes,65504 own unwritten'))
 # Real tell consumes only the mapped CachedFile record, not any retired root.
 table=glob('cached_files');index=glob('input_file');memory=state['memory']
 def read(address,size):
  for key,payload in memory.items():
   begin=int(key);payload=payload if side=='original' else bytes.fromhex(payload)
   if begin<=address and address+size<=begin+len(payload):return payload[address-begin:address-begin+size]
  raise AssertionError('missing actual file owner')
 owner=int.from_bytes(read(table+index*4,4),'little');fields=struct.unpack('<10I',read(owner,40));assert fields[1]&8 and fields[7]==4944
 recorded[-1]['mapped_tell']=dict(file_index=index,owner=owner,flags=fields[1],position=fields[7],unmapped_branch_unreached=True)
def ref(file):return dict(path=str(file.relative_to(BINARY)),sha256=sha(file))
result=dict(schema=1,target_sha256=sha(path),finite_intervals=rows,actual_alias_generations=recorded,
 no_reached_old_root_read_compare_free_serialization_order_or_unknown_escape=True,scope='Existing reviewed sort/lifetime prefix plus full85B initializer and147B writer continuation through real write_subsection/index0 stub; tell uses actual mapped CachedFile position. Unreached buffered_write/fatal paths remain static-only.',
 initialized_output='Two actual DWORD writes only, from contextual NB11+zero directory field. No copied or compared malloc spare capacity.',
 FP_scope='Prior exact027f/status0/all-empty sort contract retained; actual per-side full rawFP/error state additionally preserved at malloc call/return and next frontier. New app interval is integer-only; ordinary public malloc takes only capacity, not rawFP payload. No global/liveFP rule.',
 reopen_rule='Any later application stale-root dereference/compare/free/serialization/order/unknown escape reopens proof boundary; no cross-side stale-to-new equality or allocator policy adjustment.',
 source=ref(BINARY/'src/writer.c'),policy=ref(BINARY/config['main_free_policy']['path']),controller=ref(BINARY/'scripts/output-buffer-state.py'),old_lifetime_controller=ref(BINARY/'scripts/main-free-state.py'),raw_decoder=ref(BINARY/'scripts/raw-state.py'),original_observer=ref(BINARY/'scripts/output-buffer-observer.inc'),native_observer=ref(BINARY/'scripts/gdb-state.py'),checker=ref(Path(__file__)),original_receipt=ref(pair/'original/receipt.json'),native_receipt=ref(pair/'native/receipt.json'),audit=ref(pair/'final-audit/audit.json'),pair_check=ref(pair/'final-pair-check.log'),controls=ref(pair/'buffer-checks.json'))
(pair/'buffer-lifetime-review.json').write_text(json.dumps(result,indent=2)+'\n');print(sha(pair/'buffer-lifetime-review.json'))
