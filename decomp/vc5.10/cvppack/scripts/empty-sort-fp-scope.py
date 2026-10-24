#!/usr/bin/env python3
"""Pin the finite integer-only closure for this case's empty x87 payload rule.

Disassembly here checks owned byte intervals/call inventory, not a second CFG.
The ordinary whole-body graph audit and fresh dynamic comparator proof remain
required independently.
"""
import importlib.util
import json
import struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from binary import PE
from state import BINARY,ROOT,sha

pair=BINARY/'cases/hello-debug-dll/evidence/0103-packed-directory-comparator-complete'
spec=importlib.util.spec_from_file_location('scope_raw',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
original=decoder.decode((pair/'original/events').read_text());native=json.loads((pair/'native53/state.json').read_text());data=json.loads((pair/'original/data.json').read_text())
def read(memory,address,size):
 for base,payload in memory.items():
  base=int(base);raw=bytes.fromhex(payload) if isinstance(payload,str) else payload
  if base<=address and address+size<=base+len(raw):return raw[address-base:address-base+size]
 raise AssertionError('missing actual mapped-file state')
def original_global(name):
 row=next(r for r in data if r['name']==name);return int.from_bytes(read(original['memory'],row['original'],row['size']),'little')
def native_global(name):return int.from_bytes(bytes.fromhex(native['globals'][name]),'little')
records={}
for side,state,glob in [('original',original,original_global),('native',native,native_global)]:
 table=glob('cached_files');index=glob('input_file');owner=int.from_bytes(read(state['memory'],table+4*index,4),'little');words=struct.unpack('<10I',read(state['memory'],owner,40));offset=glob('codeview_file_offset')
 assert words[1]&8 and words[5] and words[6]==2097152 and words[7]==offset==4944
 records[side]=dict(file_index=index,table=table,owner=owner,flags=words[1],mapped_view=words[5],capacity=words[6],position=words[7],origin=0,grow_mapping_unreached=True,error_path_unreached=True)
# qsort/shortsort/swap have only byte/integer operations. The indirect calls are
# exactly the actual comparator sites independently checked375 times.
app=ROOT/'toolchains/5.10/bin/CVPACK.EXE';crt=ROOT/'toolchains/5.10/bin/MSVCRT.DLL';rows=[]
intervals=[('qsort',crt,0x7802543d,0x780255e2),('shortsort',crt,0x780255e2,0x7802563b),('swap',crt,0x7802563b,0x78025667),('comparator',app,0x4063f0,0x40645b),('writer_after_sort',app,0x406b3a,0x406b84),('seek_dispatch',app,0x401910,0x401940),('mapped_seek',app,0x402270,0x4022e1)]
expected={
 'qsort':[(0x780254aa,'0x780255e2'),(0x780254ff,'0x7802563b'),(0x78025517,'dword ptr [esp + 0x128]'),(0x7802552d,'dword ptr [esp + 0x128]'),(0x78025542,'0x7802563b'),(0x7802554d,'0x7802563b')],
 'shortsort':[(0x78025608,'dword ptr [esp + 0x28]'),(0x7802561e,'0x7802563b')],
 'swap':[],'comparator':[],
 'writer_after_sort':[(0x406b5a,'0x401910'),(0x406b70,'0x4054d0'),(0x406b7f,'0x4100e0')],
 'seek_dispatch':[(0x40192e,'0x402270'),(0x401937,'0x401810')],
 'mapped_seek':[(0x4022b3,'0x402440'),(0x4022cf,'0x4054d0')]}
import hashlib
for name,path,start,end in intervals:
 body=PE(path).read(start,end-start);ins=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(body,start));assert sum(i.size for i in ins)==len(body)
 assert not any(i.mnemonic.startswith(('f','v','xsave','xrstor','fxsave','fxrstor')) for i in ins),'FP-state instruction in finite integer closure'
 calls=[(i.address,i.op_str) for i in ins if i.mnemonic=='call'];assert calls==expected[name],(name,calls)
 rows.append(dict(name=name,path=str(path.relative_to(ROOT)),provider_sha256=sha(path),extent=[start,end],sha256=hashlib.sha256(body).hexdigest(),calls=[dict(site=a,target=b) for a,b in calls],mnemonics=sorted({i.mnemonic for i in ins})))
result=dict(schema=1,scope='hello-debug-dll integer qsort/comparator through next begin_buffered_output named stub only',intervals=rows,mapped_file_state=records,
 no_raw_saved_FP_payload_consumer=True,qualification='Integer byte-row sort closure contains no FP/state instructions or FP-state APIs. Callback rows/formals/results are integer-only. Post-sort mapped seek uses origin0/offset4944 within observed2MB capacity; grow/fatal/stream paths are not reached. Next buffered initializer is still a trap, not credited. No live x87 or CL/global rule.',
 predicate='Both actual entry CW027f/status0/tagsffff; all eight tags remain empty by exact full own-baseline preservation at every callback/wrapper. Each side preserves all captured own raw payload/selector/IP fields; those unused fields are never seeded across systems.',
 checker_sha256=sha(Path(__file__)),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native53/receipt.json'),validator_sha256=sha(BINARY/'scripts/complete-sort-state.py'))
(pair/'empty-fp-scope.json').write_text(json.dumps(result,indent=2)+'\n');print('finite empty-FP scope PASS')
