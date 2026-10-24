from pathlib import Path
import json,re,struct,sys,importlib.util,subprocess
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('p',B/'scripts/native-proof.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
elf=B/'build/native/c2';syms=p.symbols(elf,data=True);base,size=syms['write_record_initializers'];got=base+12+0x2c828
assert syms['c2_output_name'][0]==got+0x8d4 and syms['c2_output_name'][0]!=syms['c2_codeview_types'][0]
seam=p.verify_callsite(elf,'write_record_initializers',7,bytes.fromhex('e8000000005b81c328c80200'))
field=p.verify_callsite(elf,'write_record_initializers',0x50,bytes.fromhex('8b40048b4832'))
call=p.verify_callsite(elf,'write_record_initializers',0x56,b'\xe8'+struct.pack('<i',syms['emit_initializer'][0]-base-0x5b))
guard=p.verify_callsite(elf,'write_record_initializers',0x37,b'\x8d\x80'+struct.pack('<i',syms['c2_output_name'][0]-got)+bytes.fromhex('8338007425'))
ret=p.verify_callsite(elf,'write_record_initializers',size-1,b'\xc3')
original=S/'numeric-control-flow/initializers104-original';native=S/'numeric-control-flow/initializers104-native';masked=reader(original/'C2.EXE');source=reader(TARGET)
assert masked(0x44a6e4,0xb9)==b'\xcc'*0xb9 and masked(0x471a98,35)==b'\xcc'*35
assert source(0x44a74c,1)==source(0x471aba,1)==b'\xc3'
assert source(0x4540bc,3)==bytes.fromhex('896832')
text=subprocess.check_output(['objdump','-d','-Mintel','--disassemble=emit_initializer',str(elf)],text=True);assert len(re.findall(r'\bcc\s+int3',text))==1
assert re.search(r'\bmov\s+DWORD PTR \[ebp-0x[0-9a-f]+\],ecx',text)
q=S/'initializers104-emit-initializer-native.txt';q.write_text(text)
value={'canonical_sha256':sha((B/'build/c2.exe').read_bytes()),'native_sha256':sha(elf.read_bytes()),'source_alias':{'historical':'@write_codeview_records@4','actual_MAP':'@write_record_initializers@4','native':'write_record_initializers'},'native_GOT':got,'native_pic_seed':seam,'native_output_name_guard':guard,'native_actual_initializer_owner_and_ECX':field,'native_direct_existing_child':call,'native_RET0':ret,'existing_emit_child':{'entry':0x44a6e4,'hot':[0x44a6e4,0x44a79d],'cold':[[0x471a98,0x471abb]],'normal_RET0':[0x44a74c,0x471aba],'all_masked':True,'native_symbol':syms['emit_initializer'],'native_disassembly_sha256':sha(q.read_bytes()),'body_credit':0},'raw':{str(x/'receipt.json'):sha((x/'receipt.json').read_bytes())for x in [original,native]},'scope':'Whole37B producer + exact original/mapped native ECX pointer call. Field retains preexisting28B initializer identity and110B pack2 enclosing symbol layouts; no initializer bytes/default claimed. ActualNULL record list skips read/call, nextwrite_assembly_records(NULL).'}
(S/'initializers104-abi-packet.json').write_text(json.dumps(value,indent=2)+'\n')
