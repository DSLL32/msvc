#!/usr/bin/env python3
"""Per-owner resolver selection controls; static synthetic PE32 inputs only."""
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
 return dict(schema=1,functions={'_caller':dict(register_imports=dict(enabled=True,require_all_register_calls_resolved=True))},register_imports=dict(preservation_contracts=[contract({'import':dict(dll='msvcrt.dll',name='Foo')})]))
results=[]
def run(label,left,right,expected=True,meta=None,flags=None,error=None,accepted=None,missing_public=False,profile="cl"):
 left=image(left);right=image(right);(root/'original.exe').write_bytes(left);(root/'rebuilt.exe').write_bytes(right)
 # Code lengths are explicit, not inferred from RET-valued immediate bytes.
 length_l=next(i for i in range(0x200,0x280)if left[i]==0xcc)-0x200
 length_r=next(i for i in range(0x200,0x280)if right[i]==0xcc)-0x200
 cfg=dict(target=dict(path=str(root/'original.exe'),sha256=sha(left)),accepted=accepted if accepted is not None else ['_caller'],driver=[dict(name='_caller',start=0x401000,end=0x401000+length_l),dict(name='_helper',start=0x401080,end=0x401081)])
 (root/'config.json').write_text(json.dumps(cfg));(root/'metadata.json').write_text(json.dumps(meta if meta is not None else base()))
 publics=[('_caller',0x401000),('_caller_end',0x401000+length_r),('_helper',0x401080),('_helper_end',0x401081)]
 if missing_public:publics=[(('_missing'if n=='_helper'else n),v)for n,v in publics]
 (root/'fixture.map').write_text(''.join(' 0001:%08x %s %08x\n'%(v-0x400000,k,v)for k,v in publics))
 cmd=[str(binary),'--profile',profile,'--repo',str(root),'--project',str(root),'--original',str(root/'original.exe'),'--rebuilt',str(root/'rebuilt.exe'),'--config',str(root/'config.json'),'--runtime',str(root/'runtime.json'),'--rebuilt-runtime',str(root/'runtime.json'),'--metadata',str(root/'metadata.json'),'--map',str(root/'fixture.map')]
 cmd+=flags if flags is not None else ['--check-imports','--check-ret','--check-extents','--check-ownership']
 started=time.perf_counter();r=subprocess.run(cmd,capture_output=True,text=True);elapsed=time.perf_counter()-started
 assert(r.returncode==0)==expected,(label,r.stderr)
 if error:assert error in r.stderr,(label,r.stderr,error)
 report=json.loads(r.stdout)if r.returncode==0 else None
 results.append(dict(case=label,accepted=r.returncode==0,seconds=elapsed,diagnostic=r.stderr.strip(),function=report['functions']['_caller']if report else None));return report
positive=run('owner_only_cached_equals_direct',seed+call+ret,direct+ret)
assert positive['register_import_inputs']['enabled_functions']==['_caller']
assert 'register_imports'in positive['checks']
assert positive['functions']['_caller']['register_imports']['original']['resolved_register_calls']
r=run('other_selected_body_keeps_graph_without_resolution',seed+call+ret,direct+ret,accepted=['_caller','_helper'])
assert r['checked_functions']==2 and 'register_imports'not in r['functions']['_helper']
r=run('global_OR_enables_all_selected_bodies',seed+call+ret,direct+ret,accepted=['_caller','_helper'],flags=['--check-imports','--resolve-register-imports'])
assert r['register_import_inputs']['enabled_functions']==['_caller','_helper'] and 'register_imports'in r['functions']['_helper']
for label,value in [('false',False),('null',None),('string','true'),('number',1)]:
 m=base();m['functions']['_caller']['register_imports']['enabled']=value
 run('reject_'+label+'_owner_policy',seed+call+ret,direct+ret,False,meta=m,error='enabled must be true'if value in[False,None]else'invalid type')
m=base();m['functions']['_caller']['register_imports']['enabled']=False
run('false_cannot_disable_global',seed+call+ret,direct+ret,False,meta=m,flags=['--check-imports','--resolve-register-imports'],error='enabled must be true')
run('owner_requires_check_imports',seed+call+ret,direct+ret,False,flags=[],error='requires --check-imports')
run('owner_rejects_legacy_mode',seed+call+ret,direct+ret,False,flags=['--discovery','legacy'],error='require --discovery cfg')
run('owner_rejects_integrated_cvpack_profile',seed+call+ret,direct+ret,False,profile='cvpack',error='require a standalone CFG profile')
m=base();m['functions']['_typo']=m['functions'].pop('_caller')
run('unknown_owner_cannot_silently_skip_resolution',seed+call+ret,direct+ret,False,meta=m,error='not selected/accepted')
m=base();m['functions']['_helper']=m['functions'].pop('_caller')
run('unaccepted_owner_rejected',seed+call+ret,direct+ret,False,meta=m,error='not selected/accepted')
run('missing_paired_owner_mapping_rejected',seed+call+ret,direct+ret,False,meta=m,accepted=['_caller','_helper'],missing_public=True,error='missing from paired indexes')
m=base();m['functions']['_caller']['register_imports']['seed']='invented'
run('owner_does_not_grant_seeds',seed+call+ret,direct+ret,False,meta=m,error='unknown field `seed`')
run('owner_preserves_strict_unresolved_rejection',seed+bytes.fromhex('66be0000')+call+ret,direct+ret,False,error='unresolved register import calls')
m=base();m['register_imports']['preservation_contracts'][0]['evidence']['sha256']='0'*64
run('owner_requires_bound_evidence',seed+call+call+ret,direct+direct+ret,False,meta=m,error='stale preservation evidence')
m=base();m['register_imports']['preservation_contracts']=[]
run('owner_does_not_infer_call_preservation',seed+call+call+ret,direct+direct+ret,False,meta=m,error='unresolved register import calls')
m=base();m['functions']['_caller']['register_imports']={'enabld':True}
run('typo_policy_key_without_enabled_rejected',direct+ret,direct+ret,False,meta=m,error='unknown field `enabld`')
m=base();m['functions']['_caller']['register_imports']=[]
run('malformed_policy_section_rejected',direct+ret,direct+ret,False,meta=m,error='policy must be an object')
m=base();m['functions']['_caller']['register_imports']={'require_all_register_calls_resolved':True}
r=run('old_require_all_only_does_not_enable',direct+ret,direct+ret,meta=m)
assert 'register_import_inputs'not in r and 'register_imports'not in r['functions']['_caller']
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=sha(binary.read_bytes()),results=results),indent=2)+'\n')
print(f'Owner-enable CLI: {len(results)} controls passed.')
