from pathlib import Path
import sys,json,copy,tempfile,time,subprocess,hashlib,importlib
root=Path('/root/msvc2/decomp/vc5.10/link');sys.path.insert(0,str(root/'scripts'))
from binary import PE
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
module=importlib.import_module('full-link-preservation');original=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');rebuilt=PE(root/'build/link.exe');mapping=(root/'build/link.map').read_text();config=json.loads((root/'build/trial-seek-file.json').read_text())
packet=json.loads(module.PACKET.read_text());owner,=[x for x in packet['function_contracts']if x['name']=='_seek_file@16'];data=bytes.fromhex(owner['rebuilt']['bytes']);address=module.publics(mapping)['_seek_file@16'];cs=Cs(CS_ARCH_X86,CS_MODE_32);instructions=list(cs.disasm(data,0));out=Path('/root/.ai-tmp/msvc2/vc5.10/link/seek-counterpart-controls-final');out.mkdir();checks=[];started=time.perf_counter()
module.check(original,rebuilt,mapping,config)
def run(label,body=None,field=None,expected=''):
 p=copy.deepcopy(packet);r,=[x for x in p['function_contracts']if x['name']=='_seek_file@16'];image=copy.copy(rebuilt)
 if body is not None:
  offset=rebuilt.offset(address);image.data=rebuilt.data[:offset]+body+rebuilt.data[offset+len(body):];r['rebuilt']['bytes']=body.hex()
 if field:field(r['rebuilt'])
 path=out/(label+'.json');path.write_text(json.dumps(p,indent=2)+'\n');old=module.PACKET;module.PACKET=path
 try:
  try:module.check(original,image,mapping,config)
  except AssertionError as error:
   assert expected in str(error),(label,str(error));checks.append(dict(label=label,expected=expected,actual=str(error)));return
  raise AssertionError('mutation accepted '+label)
 finally:module.PACKET=old
changed=bytearray(data);position=data.index(bytes.fromhex('8b45fc'));changed[position+1]=0x7d
run('coherent-physical-edi-clobber',bytes(changed),expected='writes EDI')
changed=bytearray(data);changed[-2]=12
run('coherent-physical-ret12',bytes(changed),expected='RET ABI')
changed=bytearray(data);branch=next(i for i in instructions if i.mnemonic.startswith('j')and len(i.bytes)==2);changed[branch.address+1]=127
run('coherent-local-branch-escape',bytes(changed),expected='branch escapes')
run('truncated-current-body',data[:-1],expected='complete body')
run('wrong-owned-body-size',field=lambda r:r.__setitem__('complete_bytes',95),expected='complete body')
run('wrong-declared-return',field=lambda r:r.__setitem__('returns',[dict(offset=91,cleanup=12)]),expected='RET ABI')
run('invented-own-write-metadata',field=lambda r:r.__setitem__('own_writes',[0]),expected='writes EDI')
validation=json.loads((Path('/root/.ai-tmp/msvc2/vc5.10/link/native-k_jrvlrb')/'acceptance-validation.json').read_text());cmd=validation['audit']['full_link_import_audit']['command'];index=cmd.index('--metadata')+1;metadata=json.loads(Path(cmd[index]).read_text());contracts=metadata['register_imports']['preservation_contracts'];removed=[c for c in contracts if c.get('target',{}).get('function')=='_seek_file@16'];assert len(removed)==1
metadata['register_imports']['preservation_contracts']=[c for c in contracts if c not in removed];path=out/'remove-seek-edi-contract.json';path.write_text(json.dumps(metadata,indent=2)+'\n');command=cmd.copy();command[index]=str(path);result=subprocess.run(command,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.PIPE);(out/'remove-seek-edi-contract.stdout').write_bytes(result.stdout);(out/'remove-seek-edi-contract.stderr').write_bytes(result.stderr);assert result.returncode and b'unresolved register import calls'in result.stderr;checks.append(dict(label='remove-necessary-seek-EDI-contract',status=result.returncode,stderr=result.stderr.decode()))
receipt=dict(positive=True,negative_count=len(checks),rejected=checks,elapsed_seconds=time.perf_counter()-started,checker_sha256=hashlib.sha256((root/'scripts/full-link-preservation.py').read_bytes()).hexdigest(),packet_sha256=hashlib.sha256(module.PACKET.read_bytes()).hexdigest(),qualification='Seven focused finite whole-body controls including coherent physical EDI/RET/branch mutations, plus affected common-contract necessity. Unchanged other31 ordinary/provider masks/callback premises use retained historical review, no replay.')
(out/'validation.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
