import sys,importlib,json,copy,hashlib,subprocess,time
from pathlib import Path
root=Path('/root/msvc2/decomp/vc5.10/link');sys.path.insert(0,str(root/'scripts'));from binary import PE
checker=importlib.import_module('module-lookup-preservation');original=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(root/'build/link.exe');scratch=Path('/root/.ai-tmp/msvc2/vc5.10/link/lookup99-imports');scratch.mkdir(exist_ok=True)
start=time.perf_counter();binding=checker.check(original,rebuilt,(root/'build/link.map').read_text());config=json.loads((root/'build/lookup99-trial.json').read_text());config['accepted']=[checker.OWNER];(scratch/'config.json').write_text(json.dumps(config,indent=2)+'\n');metadata=json.loads((root/'build/graph.json').read_text());metadata['functions'].setdefault(checker.OWNER,{})['register_imports']={'require_all_register_calls_resolved':True};metadata['register_imports']={'preservation_contracts':checker.contracts()};(scratch/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n')
command=[str(root.parents[2]/'tooling/graph-audit/target/release/graph-audit'),'--profile','link','--discovery','cfg','--repo',str(root.parents[2]),'--project',str(root),'--original',str(root.parents[2]/'toolchains/5.10/bin/LINK.EXE'),'--rebuilt',str(root/'build/link.exe'),'--map',str(root/'build/link.map'),'--runtime',str(root/'runtime.json'),'--rebuilt-runtime',str(root/'build/runtime.json'),'--config',str(scratch/'config.json'),'--metadata',str(scratch/'metadata.json'),'--check-ret','--check-imports','--resolve-register-imports']
run=subprocess.run(command,capture_output=True,text=True);(scratch/'positive.stdout').write_text(run.stdout);(scratch/'positive.stderr').write_text(run.stderr);assert run.returncode==0,run.stderr;report=json.loads(run.stdout);inventory=checker.check_resolution(report,rebuilt);rejected=[]
for i,c in enumerate(checker.contracts()):
 changed=copy.deepcopy(metadata);changed['register_imports']['preservation_contracts'].pop(i);path=scratch/('remove%d.json'%i);path.write_text(json.dumps(changed,indent=2)+'\n');cmd=command.copy();cmd[cmd.index('--metadata')+1]=str(path);r=subprocess.run(cmd,capture_output=True,text=True);(scratch/('remove%d.stdout'%i)).write_text(r.stdout);(scratch/('remove%d.stderr'%i)).write_text(r.stderr);assert r.returncode!=0 and'unresolved register'in(r.stdout+r.stderr).lower(),r.stderr;rejected.append(dict(contract=c,reason=r.stderr))
packet=json.loads(checker.PACKET.read_text());witness=[]
for label,mutate in [('originalrange',lambda p:p['function_contract']['original'].__setitem__('end',0x40fbcd)),('EBXwriteclaim',lambda p:p['function_contract']['original'].__setitem__('own_ebx_writes',[0x40fb90])),('originalRETclaim',lambda p:p['function_contract']['original']['returns'][0].__setitem__('bytes','c20800')),('counterpartbody',lambda p:p['function_contract']['rebuilt'].__setitem__('bytes','90')),('counterpartEBXclaim',lambda p:p['function_contract']['rebuilt'].__setitem__('own_ebx_writes',[0])),('provideridentity',lambda p:p['imports'][0].__setitem__('provider_export',dict(ordinal=1,rva=1))),('cachedsite',lambda p:p.__setitem__('original_cached_sites',[0x40f7eb]))]:
 changed=copy.deepcopy(packet);mutate(changed)
 try:checker.check(original,rebuilt,(root/'build/link.map').read_text(),changed)
 except(AssertionError,KeyError):witness.append(label)
 else:raise AssertionError(label+' accepted')
# Genuine body mutation with matching new hash cannot hide an EBX write.
raw=bytearray(original.read(0x40fb90,60));assert raw[4:7].hex()=='8b5008';raw[5]=0x58
class OriginalMutation:
 data=original.data
 def read(self,address,size):return bytes(raw)if(address,size)==(0x40fb90,60)else original.read(address,size)
changed=copy.deepcopy(packet);changed['function_contract']['original']['sha256']=hashlib.sha256(raw).hexdigest()
try:checker.check(OriginalMutation(),rebuilt,(root/'build/link.map').read_text(),changed)
except AssertionError:witness.append('physical originalEBXclobber with refreshedbodyhash')
else:raise AssertionError('physicalEBXclobber accepted')
receipt=dict(binding=binding,inventory=inventory,necessary_contract_rejections=rejected,focused_witness_rejections=witness,elapsed_seconds=time.perf_counter()-start,command=command,packet_sha256=hashlib.sha256(checker.PACKET.read_bytes()).hexdigest(),checker_sha256=hashlib.sha256(Path(checker.__file__).read_bytes()).hexdigest());(scratch/'controls.json').write_text(json.dumps(receipt,indent=2)+'\n');print('lookup import',inventory,'2 necessary contracts +',len(witness),'witness negatives PASS',receipt['elapsed_seconds'])
