from pathlib import Path
import json,re,struct,sys,importlib.util,subprocess
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('p',B/'scripts/native-proof.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
elf=B/'build/native/c2';syms=p.symbols(elf,data=True);base,size=syms['begin_function_input'];got=base+12+0x2d018
seed=p.verify_callsite(elf,'begin_function_input',7,bytes.fromhex('e8000000005b81c318d00200'))
assert syms['code_generation_state'][0]==got+0x4274 and syms['current_function'][0]==got+0x4264 and syms['function_input_flags'][0]==got+0x4280 and syms['function_analysis_available'][0]==got+0x4284
calls=[]
for at,name in [(0x2e,'reset_register_marks'),(0x36,'initialize_function_records'),(0x3e,'clear_function_input_state'),(0x73,'reset_function_listing')]:
 calls.append({'name':name,'bytes':p.verify_callsite(elf,'begin_function_input',at,b'\xe8'+struct.pack('<i',syms[name][0]-base-at-5))})
codegen=p.verify_callsite(elf,'begin_function_input',0x46,bytes.fromhex('8d8874420000c70101000000'))
byte=p.verify_callsite(elf,'begin_function_input',0x52,bytes.fromhex('c6808042000000'))
current=p.verify_callsite(elf,'begin_function_input',0x59,bytes.fromhex('8d8864420000c70100000000'))
analysis=p.verify_callsite(elf,'begin_function_input',0x7b,bytes.fromhex('c7838442000000000000'))
ret=p.verify_callsite(elf,'begin_function_input',size-1,b'\xc3')
O=S/'numeric-control-flow/begin106-original';N=S/'numeric-control-flow/begin106-native';masked=reader(O/'C2.EXE');rows=[];v=json.loads((B/'cases/analysis/begin-function-input-analysis.json').read_text())
for child in v['children']:
 a,z=child['analysis'];assert masked(a,z-a)==b'\xcc'*(z-a)
 text=subprocess.check_output(['objdump','-d','-Mintel','--disassemble='+child['native'],str(elf)],text=True);assert len(re.findall(r'\bcc\s+int3',text))==1
 q=S/('begin106-'+child['native']+'-native.txt');q.write_text(text);rows.append({'name':child['name'],'original_whole_analysis_masked':[a,z],'native_symbol':syms[child['native']],'native_disassembly_sha256':sha(q.read_bytes()),'body_credit':0})
value={'canonical_sha256':sha((B/'build/c2.exe').read_bytes()),'native_sha256':sha(elf.read_bytes()),'native_GOT':got,'native_pic_seed':seed,'native_calls':calls,'native_shared_code_generation_state':codegen,'native_existing_current_function':current,'native_exact_lowbyte_clear':byte,'native_analysis_flag_clear':analysis,'native_RET0':ret,'children':rows,'raw':{str(x/'receipt.json'):sha((x/'receipt.json').read_bytes())for x in [O,N]},'scope':'Whole76Bwrapper source; allfourchildren stillmasked. Current actualfirstnoargreset trap precedes allscalarwrites, so observedstatesall0; auxiliarynonzero tests separatelyprove upper3bytes/orderedcalls/physicalaliases.'}
(S/'begin106-abi-packet.json').write_text(json.dumps(value,indent=2)+'\n')
