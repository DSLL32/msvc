from pathlib import Path
import json,re,struct,subprocess,sys,importlib.util
B=Path('/root/msvc2/decomp/vc5.10/c2');S=Path('/root/.ai-tmp/msvc2/vc5.10/c2');sys.path.insert(0,str(B/'scripts'))
from state import TARGET,sha
from data_evidence import reader
spec=importlib.util.spec_from_file_location('np',B/'scripts/native-proof.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
contract=json.loads((B/'cases/analysis/symbol-callback-contract.json').read_text());c=json.loads((B/'functions.json').read_text());maps={m[1]:int(m[2],16)for m in re.finditer(r'^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})',(B/'build/c2.map').read_text(),re.M)}
pe=reader(B/'build/c2.exe');elf=B/'build/native/c2';symbols=p.symbols(elf,data=True);owner='write_pending_debug_symbols';start,size=symbols[owner];got=start+12+0x2b998
assert symbols['output_slot_497cec'][1]==4 and got+0x42ec==symbols['output_slot_497cec'][0]and got+0x4290==symbols['pending_debug_symbols'][0]
seed=p.verify_callsite(elf,owner,7,bytes.fromhex('e8000000005b81c398b90200'))
site=p.verify_callsite(elf,owner,0x3a,bytes.fromhex('8d83ec4200008b008b8b904200008b4904ffd0'))
ret=p.verify_callsite(elf,owner,size-1,b'\xc3')
original=S/'numeric-control-flow/symbols102-original';native=S/'numeric-control-flow/symbols102-native';masked=reader(original/'C2.EXE');target_rows=[]
for target in contract['targets']:
 assert target['logical']not in c['accepted']+c['trial'];r=next(r for r in c['stubs']if r['name']==target['logical']);assert r['pending_extent']and not r['pending_signature']and r['end']-r['start']==1
 at=maps[target['symbol']];code=pe(at,18)
 assert code==bytes.fromhex('558bec51535657894dfccc5f5e5b8be55dc3'),'canonical typedstub must saveECX/INT3/RET0'
 assert all(masked(a,z-a)==b'\xcc'*(z-a)for a,z in [target['primary']]+target['code_chunks']),'knownoriginal body must remain completely masked'
 decoded=subprocess.check_output(['objdump','-d','-Mintel','--disassemble='+target['native'],str(elf)],text=True);traps=re.findall(r'^\s*([0-9a-f]+):\s+cc\s+int3\s*$',decoded,re.M);returns=re.findall(r'^\s*([0-9a-f]+):\s+c3\s+ret\s*$',decoded,re.M)
 assert len(traps)==len(returns)==1
 # The saved incomingECX slot and actual INT3 are both in the named target,
 # independent of tracing's temporary clobbers. Full emitted code is retained.
 assert re.search(r'\bmov\s+DWORD PTR \[ebp-0x[0-9a-f]+\],ecx',decoded)
 out=S/('symbols102-'+target['native']+'-native.txt');out.write_text(decoded)
 target_rows.append({'logical':target['logical'],'canonical_symbol':target['symbol'],'canonical_start':at,'canonical_bytes':code.hex(),'native_symbol':target['native'],'native_start':symbols[target['native']][0],'native_size':symbols[target['native']][1],'native_INT3':int(traps[0],16),'native_RET0':int(returns[0],16),'native_disassembly_path':str(out),'native_disassembly_sha256':sha(out.read_bytes()),'masked_original_ranges':[target['primary']]+target['code_chunks'],'body_credit':0})
old=json.loads((S/'numeric-control-flow/align101-original/base_configuration.json').read_text());oldboundary=(B/old['output_callback_boundary']).read_bytes();assert old['output_callback_boundary_sha256']==c['output_callback_boundary_sha256']==sha(oldboundary)
# The finite original slot/target identity matrix never changes when adding ABI.
assert json.loads(oldboundary)['slots']==json.loads((B/c['output_callback_boundary']).read_text())['slots']
v={'configuration_sha256':sha((B/'functions.json').read_bytes()),'contract_sha256':c['symbol_callback_contract_sha256'],'source_hashes':{str(p.relative_to(B)):sha(p.read_bytes())for p in [B/'src/output-callbacks.h',B/'src/output-callbacks.c',B/'src/code-records.c',B/'src/stubs.c']},'canonical_sha256':sha((B/'build/c2.exe').read_bytes()),'native_sha256':sha(elf.read_bytes()),'native_GOT':got,'native_consumer_seed':seed,'native_qualifiedslot_symbol_ECX_call':site,'native_RET0':ret,'targets':target_rows,'raw':{str(root/'receipt.json'):sha((root/'receipt.json').read_bytes())for root in [original,native]},'scope':'Both actual typedstubs stillINT3 with genuineECX formal/RET0, no wrapper/newbody; entire original targethot/cold bytes remainCC. One finite497cec consumer only; all20 originalslotidentities unchanged. CurrentqueueNULL means neither callbackexecuted; complete original/native pair matches real subsequent noarg41f559 frontier.'}
(S/'symbols102-abi-packet.json').write_text(json.dumps(v,indent=2)+'\n')
