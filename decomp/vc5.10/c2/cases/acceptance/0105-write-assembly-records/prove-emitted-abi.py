from pathlib import Path
import json,re,struct,sys,importlib.util,subprocess
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('p',B/'scripts/native-proof.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
elf=B/'build/native/c2';syms=p.symbols(elf,data=True);base,size=syms['write_assembly_records'];got=base+12+0x2c5b8
assert syms['assembly_output_active'][0]==got+0x4294
seed=p.verify_callsite(elf,'write_assembly_records',7,bytes.fromhex('e8000000005b81c3b8c50200'))
guard=p.verify_callsite(elf,'write_assembly_records',0x37,bytes.fromhex('83b89442000000742f'))
serial=p.verify_callsite(elf,'write_assembly_records',0x4e,bytes.fromhex('8b40048b4828'))
lookup=p.verify_callsite(elf,'write_assembly_records',0x54,b'\xe8'+struct.pack('<i',syms['lookup_initializer'][0]-base-0x59))
forward=p.verify_callsite(elf,'write_assembly_records',0x5c,b'\x89\xc1')
emit=p.verify_callsite(elf,'write_assembly_records',0x5e,b'\xe8'+struct.pack('<i',syms['write_assembly_initializer'][0]-base-0x63))
ret=p.verify_callsite(elf,'write_assembly_records',size-1,b'\xc3')
O=S/'numeric-control-flow/assembly105-original-final';N=S/'numeric-control-flow/assembly105-native-final';masked=reader(O/'C2.EXE');actual=reader(TARGET)
assert masked(0x486b1d,31)==b'\xcc'*31 and masked(0x486b54,1)==b'\xcc'
assert masked(0x48a157,250)==b'\xcc'*250
assert masked(0x486b3c,24)==b'\xcc'*24,'separate extent-known internal-error remains entirely masked/unaccepted'
assert actual(0x486b54,1)==actual(0x48a205,1)==b'\xc3'
rows=[]
for name in ['lookup_initializer','write_assembly_initializer']:
 text=subprocess.check_output(['objdump','-d','-Mintel','--disassemble='+name,str(elf)],text=True)
 assert len(re.findall(r'\bcc\s+int3',text))==1 and re.search(r'\bmov\s+DWORD PTR \[ebp-0x[0-9a-f]+\],ecx',text)
 q=S/('assembly105-'+name+'-native.txt');q.write_text(text);rows.append(dict(name=name,native_symbol=syms[name],native_disassembly_sha256=sha(q.read_bytes()),body_credit=0))
v={'canonical_sha256':sha((B/'build/c2.exe').read_bytes()),'native_sha256':sha(elf.read_bytes()),'native_GOT':got,'native_pic_seed':seed,'native_actual_assembly_flag':guard,'native_actual_symbol_serialECX':serial,'native_lookup':lookup,'native_returned_initializer_EAX_to_ECX':forward,'native_emit':emit,'native_RET0':ret,'pending_children':rows,'original_mask':{'lookup_unaccepted_prefix':[0x486b1d,0x486b3c],'lookup_unaccepted_successRET':[0x486b54,0x486b55],'preexisting_unaccepted_internal_error_body':[0x486b3c,0x486b54],'whole250B_emitter_masked':[0x48a157,0x48a251]},'raw':{str(x/'receipt.json'):sha((x/'receipt.json').read_bytes())for x in [O,N]},'scope':'Currentassembly0 skipschildren; independent finite operand/owner/ABI proof and isolated nonzero control. Caller105full56B strictcommon audit, no newchildbody/sharedextent credit. Actualnextnoarg begin_function_input after controller selectedfirst85/flags2a/pending776/volatile128.'}
(S/'assembly105-abi-packet.json').write_text(json.dumps(v,indent=2)+'\n')
