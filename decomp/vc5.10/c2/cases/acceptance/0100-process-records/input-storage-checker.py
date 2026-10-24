from pathlib import Path
import json,re,struct,sys,importlib.util
B=Path('/root/msvc2/decomp/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('np',B/'scripts/native-proof.py');proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
c=json.loads((B/'functions.json').read_text());maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(B/'build/c2.map').read_text(),re.M)}
r=reader(TARGET);assert r(0x41f494,5)==b'\xa1'+struct.pack('<I',0x490b74);assert r(0x41f4a0,11)==b'\x8b\x0d'+struct.pack('<I',0x497458)+b'\xe8'+struct.pack('<i',0x41f4c5-0x41f4ab)
w=reader(B/'build/c2.exe');start=maps['@process_records@4'];flag=maps['_c2_zero_alignment'];root=maps['_pending_debug_symbols'];target=maps['@align_pending_debug_symbols@4']
assert w(start+7,9)==b'\x83\x3d'+struct.pack('<I',flag)+bytes.fromhex('00750b')
assert w(start+16,11)==b'\x8b\x0d'+struct.pack('<I',root)+b'\xe8'+struct.pack('<i',target-start-27)
for read,at in [(r,0x497458),(r,0x490b74),(w,root),(w,flag)]:assert read(at,4)==bytes(4)
assert root!=flag
elf=B/'build/native/c2';symbols=proof.symbols(elf,data=True);ns=symbols['process_records'][0];nt=symbols['align_pending_debug_symbols'][0]
# These are the bounded emitted PIC instructions, not graph discovery.
setup=proof.verify_callsite(elf,'process_records',7,bytes.fromhex('e8000000005b81c3a8ba0200'))
got=ns+12+0x2baa8
assert got+0x4290==symbols['pending_debug_symbols'][0]and got+0x978==symbols['c2_zero_alignment'][0]
load=proof.verify_callsite(elf,'process_records',0x45,bytes.fromhex('8b8b90420000'))
flagload=proof.verify_callsite(elf,'process_records',0x37,bytes.fromhex('8d8078090000833800750e'))
call=proof.verify_callsite(elf,'process_records',0x4b,b'\xe8'+struct.pack('<i',nt-ns-0x50))
source=(B/'src/code-records.c').read_text();assert source.count('align_pending_debug_symbols(pending_debug_symbols);')==1
v={'target_sha256':sha(TARGET.read_bytes()),'source_sha256':sha(source.encode()),'rebuilt_sha256':sha((B/'build/c2.exe').read_bytes()),'native_sha256':sha(elf.read_bytes()),'original':{'flag':0x490b74,'root':0x497458,'call':0x41f4a6,'target':0x41f4c5,'next_return':0x41f4ab},'canonical':{'entry':start,'root':root,'flag':flag,'target':target},'native':{'got':got,'root':symbols['pending_debug_symbols'][0],'flag':symbols['c2_zero_alignment'][0],'setup':setup,'flag_load':flagload,'pointer_load':load,'call':call},'scope':'Finite actual slot/load/conditional/CALL binding proves shared root identity even though both actual values are zero. Initial globals only, no allocated list-node payload claim. Native child remains named INT3/unaccepted/masked; whole100 common strict graph audit independently checks controlflow,RET,refs,extents,ownership.'}
Path('/root/.ai-tmp/msvc2/vc5.10/c2/process100-input-storage-proof.json').write_text(json.dumps(v,indent=2)+'\n')
