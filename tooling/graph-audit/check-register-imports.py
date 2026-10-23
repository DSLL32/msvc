#!/usr/bin/env python3
"""Synthetic CLI contract/dataflow mutations. No compiler, target run or Wine."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',type=Path,required=True);p.add_argument('--scratch',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
a=p.parse_args();binary=a.binary.resolve();root=a.scratch.resolve();assert root.is_relative_to(Path.home()/'.ai-tmp');root.mkdir(parents=True,exist_ok=True)
def sha(b):return hashlib.sha256(b).hexdigest()
def image(code):
 b=bytearray(0x600)
 def w(at,n):struct.pack_into('<I',b,at,n)
 b[:2]=b'MZ';w(60,0x80);b[0x80:0x84]=b'PE\0\0';struct.pack_into('<HH',b,0x84,0x14c,1);struct.pack_into('<H',b,0x94,0xe0)
 struct.pack_into('<H',b,0x98,0x10b);w(0xb4,0x400000);w(0x98+92,16);w(0x98+104,0x1100);w(0x98+108,40);b[0x178:0x17d]=b'.text'
 for off,v in [(8,0x400),(12,0x1000),(16,0x400),(20,0x200),(36,0x60000020)]:w(0x178+off,v)
 b[0x200:0x300]=b'\xcc'*0x100;b[0x200:0x200+len(code)]=code;b[0x280]=0xc3
 for at,v in [(0x300,0x1140),(0x30c,0x1180),(0x310,0x1150),(0x340,0x11c0),(0x344,0x11d0),(0x350,0x11c0),(0x354,0x11d0)]:w(at,v)
 b[0x380:0x38b]=b'msvcrt.dll\0';b[0x3c2:0x3c6]=b'Foo\0';b[0x3d2:0x3d6]=b'Bar\0';return bytes(b)
seed=bytes.fromhex('8b3550114000');call=bytes.fromhex('ffd6');direct=bytes.fromhex('ff1550114000');ret=b'\xc3'
(root/'runtime.json').write_text('[]\n');(root/'packet.json').write_text('{"fixture":"synthetic ordinary Win32 register preservation witness"}\n')
evidence=dict(path='packet.json',sha256=sha((root/'packet.json').read_bytes()))
def contract(target,preserves=['ebx','esi','edi']):return dict(target=target,preserves=preserves,evidence=copy.deepcopy(evidence))
def base():
 return dict(schema=1,functions={'_caller':dict(register_imports=dict(require_all_register_calls_resolved=True))},register_imports=dict(preservation_contracts=[contract({'import':dict(dll='msvcrt.dll',name='Foo')})]))
results=[]
def run(label,left,right,expected=True,meta=None,flags=None,error=None):
 left=image(left);right=image(right);(root/'original.exe').write_bytes(left);(root/'rebuilt.exe').write_bytes(right)
 # Code lengths are explicit, not inferred from RET-valued immediate bytes.
 length_l=next(i for i in range(0x200,0x280)if left[i]==0xcc)-0x200
 length_r=next(i for i in range(0x200,0x280)if right[i]==0xcc)-0x200
 cfg=dict(target=dict(path=str(root/'original.exe'),sha256=sha(left)),accepted=['_caller'],driver=[dict(name='_caller',start=0x401000,end=0x401000+length_l),dict(name='_helper',start=0x401080,end=0x401081)])
 (root/'config.json').write_text(json.dumps(cfg));(root/'metadata.json').write_text(json.dumps(meta if meta is not None else base()))
 publics=[('_caller',0x401000),('_caller_end',0x401000+length_r),('_helper',0x401080),('_helper_end',0x401081)]
 (root/'fixture.map').write_text(''.join(' 0001:%08x %s %08x\n'%(v-0x400000,k,v)for k,v in publics))
 cmd=[str(binary),'--profile','cl','--repo',str(root),'--project',str(root),'--original',str(root/'original.exe'),'--rebuilt',str(root/'rebuilt.exe'),'--config',str(root/'config.json'),'--runtime',str(root/'runtime.json'),'--rebuilt-runtime',str(root/'runtime.json'),'--metadata',str(root/'metadata.json'),'--map',str(root/'fixture.map')]
 cmd+=flags if flags is not None else ['--check-imports','--resolve-register-imports','--check-ret','--check-extents','--check-ownership']
 started=time.perf_counter();r=subprocess.run(cmd,capture_output=True,text=True);elapsed=time.perf_counter()-started
 assert(r.returncode==0)==expected,(label,r.stderr)
 if error:assert error in r.stderr,(label,r.stderr,error)
 report=json.loads(r.stdout)if r.returncode==0 else None
 results.append(dict(case=label,accepted=r.returncode==0,seconds=elapsed,diagnostic=r.stderr.strip(),function=report['functions']['_caller']if report else None));return report
positive=run('cached_call_equals_direct_IAT',seed+call+ret,direct+ret)
assert len(positive['functions']['_caller']['register_imports']['original']['resolved_register_calls'])==1
run('explicit_preservation_repeated_calls',seed+call+call+ret,direct+direct+ret)
run('require_import_flag',seed+call+ret,direct+ret,False,flags=['--resolve-register-imports'],error='--check-imports')
run('legacy_mode_rejects_feature',seed+call+ret,direct+ret,False,flags=['--discovery','legacy','--check-imports','--resolve-register-imports'],error='require --discovery cfg')
run('bypassed_seed',b'\x74\x06'+seed+call+ret,direct+ret,False,error='unresolved register import calls')
run('wrong_register',seed+bytes.fromhex('ffd7')+ret,direct+ret,False,error='unresolved register import calls')
run('wrong_seed_IAT_identity',bytes.fromhex('8b3554114000')+call+ret,direct+ret,False,error='import graph differs')
run('partial_SI_write',seed+bytes.fromhex('66be0000')+call+ret,direct+ret,False,error='unresolved register import calls')
run('implicit_ESI_write',seed+b'\xac'+call+ret,direct+ret,False,error='unresolved register import calls')
m=base();m['register_imports']['preservation_contracts']=[]
run('unknown_import_call_barrier',seed+call+call+ret,direct+direct+ret,False,meta=m,error='unresolved register import calls')
# helper is direct and remains a graph vertex, not an imported identity.
helper_l=bytes.fromhex('e875000000');helper_r=bytes.fromhex('e87b000000')
run('unknown_custom_direct_call_barrier',seed+helper_l+call+ret,helper_r+direct+ret,False,error='unresolved register import calls')
m=base();m['register_imports']['preservation_contracts'].append(contract(dict(function='_helper')))
run('named_direct_callee_preservation',seed+helper_l+call+ret,helper_r+direct+ret,meta=m)
for label,mutator,error in [
 ('stale_witness_digest',lambda c:c['evidence'].update(sha256='0'*64),'stale preservation evidence'),
 ('absolute_witness_path',lambda c:c['evidence'].update(path=str(root/'packet.json')),'project-relative'),
 ('missing_witness_file',lambda c:c['evidence'].update(path='not-present.json'),'resolve preservation evidence'),
 ('unsupported_preserved_register',lambda c:c.update(preserves=['eax']),'only ebx/esi/edi'),
 ('missing_named_callee',lambda c:c.update(target={'function':'_missing'}),'missing from verified index'),
 ('unknown_qualified_import',lambda c:c.update(target={'import':{'dll':'other.dll','name':'Foo'}}),'missing from verified IAT'),
]:
 m=base();mutator(m['register_imports']['preservation_contracts'][0]);run(label,seed+call+ret,direct+ret,False,meta=m,error=error)
outside=root.parent/'outside-preservation-witness.json';outside.write_text('{}\n');link=root/'outside-link.json'
if not link.exists():link.symlink_to(outside)
m=base();m['register_imports']['preservation_contracts'][0]['evidence']=dict(path=link.name,sha256=sha(outside.read_bytes()))
run('symlink_witness_escape',seed+call+ret,direct+ret,False,meta=m,error='escapes project')
m=base();m['functions']['_caller']['register_imports']['require_all_register_calls_resolved']=False
r=run('genuine_unknown_callback_stays_indirect',bytes.fromhex('ffd0c3'),bytes.fromhex('ffd0c3'),meta=m)
assert r['functions']['_caller']['edges']==['indirect']
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=sha(binary.read_bytes()),positive_report=positive,results=results),indent=2)+'\n')
print(f'Register-IAT CLI: {len(results)} controls passed.')
