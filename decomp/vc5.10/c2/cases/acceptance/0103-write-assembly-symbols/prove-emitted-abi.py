from pathlib import Path
import json,re,struct,sys,importlib.util,subprocess
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('p',B/'scripts/native-proof.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
c=json.loads((B/'functions.json').read_text());v=json.loads((B/c['assembly_visitor_contract']).read_text());elf=B/'build/native/c2';syms=p.symbols(elf,data=True);base=syms['write_assembly_symbols'][0];got=base+12+0x2c708
assert syms['assembly_output_active'][0]==got+0x4294
proof=[p.verify_callsite(elf,'write_assembly_symbols',7,bytes.fromhex('e8000000005b81c308c70200')),p.verify_callsite(elf,'write_assembly_symbols',0x2e,bytes.fromhex('83b894420000007466'))]
rows=[]
for (flag,address),(flagoffset,ptroffset,calloffset)in zip(v['passes'],[(0x3a,0x3c,0x42),(0x4a,0x4c,0x52),(0x5a,0x5f,0x65),(0x6d,0x6f,0x75),(0x7d,0x82,0x88),(0x90,0x92,0x98)]):
 name='assembly_callback_%x'%address
 flagbytes=b'\x31\xc9'if flag==0 else bytes.fromhex('b901000000')
 a=p.verify_callsite(elf,'write_assembly_symbols',flagoffset,flagbytes)
 b=p.verify_callsite(elf,'write_assembly_symbols',ptroffset,b'\x8d\x93'+struct.pack('<i',syms[name][0]-got))
 d=p.verify_callsite(elf,'write_assembly_symbols',calloffset,b'\xe8'+struct.pack('<i',syms['visit_symbols'][0]-(base+calloffset+5)))
 rows.append(dict(flag=flag,original_callback=address,native_callback=name,scope_ECX=a,code_EDX=b,actual_call=d))
original=S/'numeric-control-flow/assembly103-original-current';native=S/'numeric-control-flow/assembly103-native-final';masked=reader(original/'C2.EXE');targets=[]
for r in v['callbacks']:
 assert masked(r['entry'],1)==b'\xcc'
 name=r['name'][1:];text=subprocess.check_output(['objdump','-d','-Mintel','--disassemble='+name,str(elf)],text=True);assert len(re.findall(r'\bcc\s+int3',text))==1
 q=S/('assembly103-'+name+'-native.txt');q.write_text(text);targets.append(dict(name=name,native_bounds=syms[name],disassembly_sha256=sha(q.read_bytes()),body_credit=0,ABI='UNRESOLVED'))
assert masked(0x42ba01,62)==b'\xcc'*62
visitor=p.verify_callsite(elf,'visit_symbols',0x16,bytes.fromhex('8955f0894dec'))
assert reader(B/'build/c2.exe')(0x4019c4,23)==bytes.fromhex('558bec83ec085356578955f8894dfccc5f5e5b8be55dc3')
out=dict(canonical_sha256=sha((B/'build/c2.exe').read_bytes()),native_sha256=sha(elf.read_bytes()),contract_sha256=c['assembly_visitor_contract_sha256'],native_GOT=got,scope_guard=proof,passes=rows,visitor_entry_argument_saves=visitor,targets=targets,raw={str(x/'receipt.json'):sha((x/'receipt.json').read_bytes())for x in [original,native]},scope='Finite emitted ECX/EDX pointer setup for all six passes. Currentassembly0 skipsall. Nativevisitor preserves exactincomingformals before genuineINT3; full62Boriginalvisitor remainsCC. Callbacksignatures remainunresolved, no newcallbackABI/bodycredit.')
(S/'assembly103-abi-packet.json').write_text(json.dumps(out,indent=2)+'\n')
