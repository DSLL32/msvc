from pathlib import Path
import json,re,struct,subprocess,sys,importlib.util
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('np',B/'scripts/native-proof.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
source=(B/'src/code-records.c').read_bytes();assert source.count(b'((void (__fastcall *)(C2DebugLink *)) c2_set_argv)(head);')==1
original=reader(TARGET);rebuilt=reader(B/'build/c2.exe');maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(B/'build/c2.map').read_text(),re.M)}
assert original(0x41f9c2,7)==bytes.fromhex('8bcee8c0f1ffff')
assert original(0x41eb89,1)==b'\xc3'
assert original(0x41f9c9,20)==bytes.fromhex('8bc6c705f07d4900000000005f5e5d5b83c408c3')
assert maps['_prepare_global_records']==0x406b85 and maps['_c2_set_argv']==0x401b14
assert rebuilt(0x406e7a,8)==bytes.fromhex('8b4df0e892acffff')
assert rebuilt(0x406e82,17)==bytes.fromhex('c70530c24000000000008b45f08be55dc3')
assert rebuilt(0x401b14,4)==bytes.fromhex('558bec5d')and rebuilt(0x401b18,1)==b'\xc3'
native=B/'build/native/c2';table=p.symbols(native);start,size=table['prepare_global_records'];target,target_size=table['c2_set_argv'];call=start+0x33d
site=p.verify_callsite(native,'prepare_global_records',0x33d,b'\xe8'+struct.pack('<i',target-call-5))
setup=p.verify_callsite(native,'prepare_global_records',0x33a,bytes.fromhex('8b4dec'))
frames=[]
for side,name in [('original','globals99-original-final'),('native','globals99-native-final')]:
 root=S/'numeric-control-flow'/name
 frames.append({'side':side,'receipt':str(root/'receipt.json'),'receipt_sha256':sha((root/'receipt.json').read_bytes()),'raw_sha256':sha((root/('events.txt'if side=='original'else'gdb.stdout')).read_bytes())})
disassemblies={}
for name,path,args in [('original-caller',TARGET,['--start-address=0x41f9be','--stop-address=0x41f9dd']),('original-noop',TARGET,['--start-address=0x41eb89','--stop-address=0x41eb8a']),('canonical-caller',B/'build/c2.exe',['--start-address=0x406e7a','--stop-address=0x406e93']),('canonical-noop',B/'build/c2.exe',['--start-address=0x401b14','--stop-address=0x401b19']),('native-caller',native,['--start-address='+hex(start+0x337),'--stop-address='+hex(start+size)]),('native-noop',native,['--disassemble=c2_set_argv'])]:
 text=subprocess.check_output(['objdump','-d','-Mintel',*args,str(path)],text=True);out=S/('globals99-'+name+'-abi.txt');out.write_text(text);disassemblies[name]={'path':str(out),'sha256':sha(out.read_bytes())}
c=json.loads((B/'functions.json').read_text());old=json.loads((S/'numeric-control-flow/records98-original/base_configuration.json').read_text());noop=next(r for r in c['stubs']if r['name']=='_output_callback_41eb89');oldnoop=next(r for r in old['stubs']if r['name']=='_output_callback_41eb89')
# Only explanatory physical-alias metadata added: pending type/body and all17slots unchanged.
assert all(noop[k]==v for k,v in oldnoop.items());assert noop['pending_signature']and noop['pending_extent']
for field in ('output_callback_boundary','output_callback_boundary_sha256'):assert c[field]==old[field]
from globals_state import ROOTS
result={'target_sha256':sha(TARGET.read_bytes()),'source_sha256':sha(source),'canonical_sha256':sha((B/'build/c2.exe').read_bytes()),'native_sha256':sha(native.read_bytes()),'canonical_symbols':{'caller':maps['_prepare_global_records'],'target':maps['_c2_set_argv']},'native_call':site,'native_ECX_head_setup':setup,'observations':frames,'disassembly':disassemblies,'scope':'Only source99 direct call physical41eb89: ECXhead,zero stackargs/RET0,resultignored; same accepted support vertex, zero extra credit.17 erased app aliases retain original boundary/pending traps. Native tracing changes volatiles but preserves physical function ABI. Whole99 graphs/RET/extents/imports/refs/ownership checked independently.'}
(S/'globals99-abi-packet.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
