#!/usr/bin/env python3
"""Frozen CV5 CVPACK EBP-IAT corpus and mixed-callback completeness controls.

Static scratch PE/metadata mutations only. Never builds, executes targets, uses
Wine or discovers another CFG. command.json supplies explicit frozen inputs.
"""
import argparse
from contextlib import contextmanager
import copy
import hashlib
import json
from pathlib import Path
import statistics
import struct
import subprocess
import time
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
root=a.snapshot.resolve();assert root.is_relative_to(Path.home()/'.ai-tmp');cmd=json.loads((root/'command.json').read_text());paths={k:Path(cmd[cmd.index('--'+k)+1])for k in ['original','rebuilt','map','config','runtime','rebuilt-runtime','metadata']};assert all(p.resolve().is_relative_to(root)for p in paths.values())
original=paths['original'].read_bytes();config=json.loads(paths['config'].read_text());metadata=json.loads(paths['metadata'].read_text());assert hashlib.sha256(original).hexdigest()=='62f51b0d705d10426d5b38fc6340859d0bfc829d9d93f27d12a6caa8a2ebe6c6'
h=struct.unpack_from('<I',original,60)[0];optional=h+24;base=struct.unpack_from('<I',original,optional+28)[0];first=optional+struct.unpack_from('<H',original,h+20)[0]
def off(address):
 for i in range(struct.unpack_from('<H',original,h+6)[0]):
  s=first+40*i;start=base+struct.unpack_from('<I',original,s+12)[0];n=struct.unpack_from('<I',original,s+16)[0]
  if start<=address<start+n:return struct.unpack_from('<I',original,s+20)[0]+address-start
 raise ValueError('mutation not file-backed')
assert original[off(0x404921):off(0x404921)+2]==bytes.fromhex('ffd5')
@contextmanager
def modified():
 saved={p:p.read_bytes()for p in paths.values()}
 try:yield
 finally:
  for p,b in saved.items():p.write_bytes(b)
def roles(d):
 f=d['functions'];o=f['_main']['register_imports']['original'];r=f['_main']['register_imports']['rebuilt'];wanted={str(v)for v in [0x404726,0x404731,0x40473d,0x404749,0x404754]}
 return set(o['resolved_register_calls'])==wanted and all(v=={'dll':'msvcrt.dll','name':'free'}for v in o['resolved_register_calls'].values()) and o['unresolved_register_calls']==[0x4047fc] and not o['other_unresolved_calls'] and not r['unresolved_register_calls'] and len(r['other_unresolved_calls'])==1
results=[]
def run(label,command=cmd,expect=True,error=None,expect_roles=True,record=True):
 started=time.perf_counter();p=subprocess.run(command,capture_output=True,text=True);elapsed=time.perf_counter()-started
 assert(p.returncode==0)==expect,(label,p.stderr)
 if error:assert error in p.stderr,(label,p.stderr)
 d=json.loads(p.stdout)if p.returncode==0 else None
 complete=roles(d)if d else False
 if d:assert complete==expect_roles,(label,complete)
 if record:results.append(dict(case=label,common_graph_accepts=p.returncode==0,role_complete=complete,seconds=elapsed,diagnostic=p.stderr.strip()))
 return d,elapsed
positive,_=run('full50_EBP_and_GPA_roles')
assert positive['checked_functions']==50
q=positive['functions']['_parse_options']['register_imports']['original'];assert q['resolved_register_calls'][str(0x404921)]==dict(dll='msvcrt.dll',name='_strcmpi');assert q['resolved_register_calls'][str(0x4048fd)]==dict(dll='msvcrt.dll',name='toupper');assert not q['unresolved_register_calls']
for label,address,data,error in [
 ('partial_BP_live_cache',0x40491b,bytes.fromhex('66bd00009090'),'unresolved register import calls'),
 ('ENTER_live_EBP_cache',0x40491b,bytes.fromhex('c80000009090'),'unresolved register import calls'),
 ('POP_live_EBP_cache',0x40491b,bytes.fromhex('5d9090909090'),'unresolved register import calls'),
 ('wrong_EBP_call_register',0x404921,bytes.fromhex('ffd6'),'unresolved register import calls'),
 ('wrong_EBP_IAT_identity',0x4048de,bytes.fromhex('8b2ddc614100'),'import graph differs'),
 ('branch_bypasses_cached_seeds',0x4048d2,b'\x0f\x84'+struct.pack('<i',0x404919-(0x4048d2+6)),'unresolved register import calls'),
]:
 with modified():
  b=bytearray(original);b[off(address):off(address)+len(data)]=data;paths['original'].write_bytes(b);c=copy.deepcopy(config);c['target']['sha256']=hashlib.sha256(b).hexdigest();paths['config'].write_text(json.dumps(c));run(label,expect=False,error=error)
for label,target in [('toupper_must_preserve_EBP','toupper'),('strcmpi_must_preserve_EBP','_strcmpi')]:
 with modified():
  m=copy.deepcopy(metadata);c=next(v for v in m['register_imports']['preservation_contracts']if v['target'].get('import',{}).get('name')==target);c['preserves'].remove('ebp');paths['metadata'].write_text(json.dumps(m));run(label,expect=False,error='unresolved register import calls')
with modified():
 m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts']=[v for v in m['register_imports']['preservation_contracts']if v['target'].get('function')!='_warning'];paths['metadata'].write_text(json.dumps(m));run('unknown_actual_warning_callee_barrier',expect=False,error='unresolved register import calls')
with modified():
 m=copy.deepcopy(metadata);m['functions']['_main']['register_imports']['require_all_register_calls_resolved']=True;paths['metadata'].write_text(json.dumps(m));run('mixed_main_cannot_require_all_imports',expect=False,error='unresolved register import calls')
with modified():
 m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts']=[v for v in m['register_imports']['preservation_contracts']if v['target'].get('import',{}).get('name')!='free'];paths['metadata'].write_text(json.dumps(m));run('lost_free_sites_graph_only_scope_control',expect_roles=False)
with modified():
 m=copy.deepcopy(metadata)
 for c in m['register_imports']['preservation_contracts']:c['preserves']=[r for r in c['preserves']if r!='ebp']
 m['register_imports']['preservation_contracts']=[c for c in m['register_imports']['preservation_contracts']if c['preserves']];paths['metadata'].write_text(json.dumps(m));c=copy.deepcopy(config);c['accepted']=['_parse_options'];paths['config'].write_text(json.dumps(c));run('released_three_register_engine_cannot_resolve_EBP',[str(a.baseline.resolve()),*cmd[1:]],expect=False,error='unresolved register import calls')
# Timings include all50 graph/ABI/extent/import checks, witness hashing and JSON.
run('warmup',record=False);samples=[run('timing',record=False)[1]for n in range(7)]
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=hashlib.sha256(Path(cmd[0]).read_bytes()).hexdigest(),input_sha256={k:hashlib.sha256(p.read_bytes()).hexdigest()for k,p in paths.items()},positive_report=positive,results=results,timings=dict(median_seconds=statistics.median(samples),min_seconds=min(samples),max_seconds=max(samples),samples_seconds=samples)),indent=2)+'\n')
print('CVPACK:50functions,2parse-options cached imports, genuineGPAopaque;10commonrejections+1baselinegap+mixed-role scopecontrol;7timings passed.')
