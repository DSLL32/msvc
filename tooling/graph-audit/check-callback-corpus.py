#!/usr/bin/env python3
"""Frozen FullLink opaque-call preservation corpus, ablations and CLI timings.

Consumes existing common CFG reports; no second CFG, build, Wine or target run.
All writes are scratch-only and restored. The target separately reviews/binds the
closed-slot and ordinary callee/provider witness packets before using this proof.
"""
import argparse
from collections import Counter
from contextlib import contextmanager
import copy
import hashlib
import json
from pathlib import Path
import statistics
import struct
import subprocess
import time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--snapshot',type=Path,required=True)
p.add_argument('--baseline',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args();root=a.snapshot.resolve();assert root.is_relative_to(Path.home()/'.ai-tmp');cmd=json.loads((root/'command.json').read_text());name='?FullLink@LinkContext@@QAEHPAE@Z'
paths={key:Path(cmd[cmd.index('--'+key)+1])for key in ['config','metadata','original','rebuilt','map','runtime','rebuilt-runtime']};assert all(p.resolve().is_relative_to(root)for p in paths.values())
original=paths['original'].read_bytes();config=json.loads(paths['config'].read_text());metadata=json.loads(paths['metadata'].read_text());assert config['accepted']==[name]
def sha(b):return hashlib.sha256(b).hexdigest()
assert sha(original)=='04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579'
assert sha(paths['metadata'].read_bytes())=='a099578cb8b4782cc4bf329a0841b9dc60d6eb5ab7e055c176a26a57f9e380c4'
head=struct.unpack_from('<I',original,60)[0];opt=head+24;base=struct.unpack_from('<I',original,opt+28)[0];first=opt+struct.unpack_from('<H',original,head+20)[0]
def off(address):
    for i in range(struct.unpack_from('<H',original,head+6)[0]):
        s=first+40*i;start=base+struct.unpack_from('<I',original,s+12)[0];n=struct.unpack_from('<I',original,s+16)[0]
        if start<=address<start+n:return struct.unpack_from('<I',original,s+20)[0]+address-start
    raise ValueError('mutation outside file-backed original')
@contextmanager
def modified():
    saved={p:p.read_bytes()for p in paths.values()}
    try:yield
    finally:
        for p,b in saved.items():p.write_bytes(b)
def inventory(side):return Counter(json.dumps(v,sort_keys=True)for key in ['resolved_register_calls','direct_import_calls']for v in side[key].values())
def complete(report):
    f=report['functions'][name]['register_imports'];o=f['original'];r=f['rebuilt'];expected={str(v):{'dll':'kernel32.dll'if api=='GetTickCount'else'msvcrt.dll','name':api}for v,api in [(0x443930,'sprintf'),(0x4444ec,'fprintf'),(0x444521,'fprintf'),(0x444865,'GetTickCount'),(0x44488e,'fprintf'),(0x4448c4,'fprintf'),(0x4448d9,'GetTickCount'),(0x444915,'fprintf')]}
    assert o['resolved_register_calls']==expected and not o['unresolved_register_calls'] and not r['unresolved_register_calls']
    assert o['other_unresolved_calls']==[0x443a3c,0x444795] and r['other_unresolved_calls']==[0x409a74,0x40ae4f]
    assert len(o['direct_import_calls'])==10 and len(r['direct_import_calls'])==18 and not r['resolved_register_calls']
    assert inventory(o)==inventory(r)
    for side,site,slot in [('original',0x444795,0x47eed0),('rebuilt',0x40ae4f,0x40e318)]:
        c=f[side]['callback_preservation'];assert set(c)=={str(site)} and c[str(site)]['slot']==slot and c[str(site)]['preserves']==['edi']
    assert 'indirect'in report['functions'][name]['edges']
results=[]
def run(label,command=cmd,expect=True,error=None,roles=True,record=True):
    t=time.perf_counter();q=subprocess.run(command,capture_output=True,text=True);seconds=time.perf_counter()-t
    assert(q.returncode==0)==expect,(label,q.stderr)
    if error:assert error in q.stderr,(label,error,q.stderr)
    r=json.loads(q.stdout)if not q.returncode else None
    if r and roles:complete(r)
    if record:results.append(dict(case=label,accepted=not q.returncode,seconds=seconds,diagnostic=q.stderr.strip()))
    return r,seconds
positive,_=run('8cached_10direct_2opaque_equal18direct_2opaque')
# Every finite ordinary contract, each multibit claim, and the callback are needed.
for i,c in enumerate(metadata['register_imports']['preservation_contracts']):
    target=json.dumps(c['target'],sort_keys=True)
    with modified():
        m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts'].pop(i);paths['metadata'].write_text(json.dumps(m));run('remove '+target,expect=False,error='unresolved register import calls')
    if len(c['preserves'])>1:
        for reg in c['preserves']:
            with modified():
                m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts'][i]['preserves'].remove(reg);paths['metadata'].write_text(json.dumps(m));run('remove '+reg+' from '+target,expect=False,error='unresolved register import calls')
with modified():
    m=copy.deepcopy(metadata);m['register_imports']['callback_preservation_contracts']=[];paths['metadata'].write_text(json.dumps(m));run('remove_callback_EDI_contract',expect=False,error='unresolved register import calls')
for label,mutator,error in [
    ('wrong_original_site',lambda c:c['original'].update(site=0x444796),'absent from discovered'),
    ('wrong_rebuilt_site',lambda c:c['rebuilt'].update(site=0x40ae50),'absent from discovered'),
    ('wrong_original_slot',lambda c:c['original'].update(slot=0x47eed4),'slot/operand differs'),
    ('wrong_rebuilt_slot',lambda c:c['rebuilt'].update(slot=0x40e31c),'slot/operand differs'),
    ('wrong_existing_owner',lambda c:c.update(owner='LinkContext::PassTwo'),'owner is not audited'),
    ('wrong_live_register_mask',lambda c:c.update(preserves=['esi']),'unresolved register import calls'),
    ('stale_callback_evidence',lambda c:c['evidence'].update(sha256='0'*64),'stale preservation evidence'),
    ('missing_rebuilt_binding',lambda c:c.pop('rebuilt'),'missing field `rebuilt`'),
]:
    with modified():
        m=copy.deepcopy(metadata);mutator(m['register_imports']['callback_preservation_contracts'][0]);paths['metadata'].write_text(json.dumps(m));run(label,expect=False,error=error)
for label,address,data,error,edit in [
    ('callback_changed_to_JMP',0x444795,bytes.fromhex('ff25d0ee4700'),'not an opaque indirect call',None),
    ('callback_changed_to_IAT',0x444795,bytes.fromhex('ff1578114000'),'not an opaque indirect call',lambda c:c['original'].update(slot=0x401178)),
    ('callback_segment_prefix',0x444794,b'\x2e','requires exact unprefixed',lambda c:c['original'].update(site=0x444794)),
    ('partial_DI_after_callback',0x444879,bytes.fromhex('66bf000090'),'unresolved register import calls',None),
]:
    with modified():
        b=bytearray(original);b[off(address):off(address)+len(data)]=data;paths['original'].write_bytes(b);c=copy.deepcopy(config);c['target']['sha256']=sha(b);paths['config'].write_text(json.dumps(c))
        if edit:
            m=copy.deepcopy(metadata);edit(m['register_imports']['callback_preservation_contracts'][0]);paths['metadata'].write_text(json.dumps(m))
        run(label,expect=False,error=error)
# Old reports/defaults remain identical. The new feature performs additional work.
old=[str(a.baseline.resolve()),*cmd[1:]];old.remove('--resolve-register-imports');old.remove('--check-imports');new_off=[cmd[0],*old[1:]]
left,_=run('approved_graph_RET_refs_extents',old,roles=False);right,_=run('new_same_workload_flag_off',new_off,roles=False);assert left==right
samples={'approved_graph_RET_refs_extents':[],'new_same_workload_flag_off':[],'new_plus_qualified_import_register_callback_proof':[]}
commands=[old,new_off,cmd]
for c in commands:run('warmup',c,roles=c==cmd,record=False)
for n in range(7):
    for key,c in zip(samples,commands):samples[key].append(run('timing',c,roles=c==cmd,record=False)[1])
timings={k:dict(median_seconds=statistics.median(v),min_seconds=min(v),max_seconds=max(v),samples_seconds=v)for k,v in samples.items()}
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=sha(Path(cmd[0]).read_bytes()),baseline_sha256=sha(a.baseline.read_bytes()),inputs_sha256={k:sha(p.read_bytes())for k,p in paths.items()},positive_report=positive,import_multiplicity=inventory(positive['functions'][name]['register_imports']['original']),results=results,timings=timings),indent=2)+'\n')
print(f'FullLink:8cached+10direct+2opaque=18direct+2opaque;{len(results)} controls/7timing samples passed.')
