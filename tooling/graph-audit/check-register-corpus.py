#!/usr/bin/env python3
"""Frozen VC5 LINK cached-IAT corpus checks, mutations and repeated CLI timing.

Requires a scratch snapshot with command.json listing explicit tool inputs, a
bound witness packet, and ONLY _configure_link_options@12 selected for comparison.
Never rebuilds, runs a target, calls another graph-discovery engine or uses Wine.
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
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
root=a.snapshot.resolve();assert root.is_relative_to(Path.home()/'.ai-tmp');cmd=json.loads((root/'command.json').read_text());baseline=a.baseline.resolve();name='_configure_link_options@12'
paths={key:Path(cmd[cmd.index('--'+key)+1])for key in ['config','metadata','original','rebuilt','map','runtime','rebuilt-runtime']};assert all(p.resolve().is_relative_to(root)for p in paths.values())
original=paths['original'].read_bytes();config=json.loads(paths['config'].read_text());metadata=json.loads(paths['metadata'].read_text());assert config['accepted']==[name]
assert hashlib.sha256(original).hexdigest()=='04c892989f4cc8076ef45e033215eb7aefbdf1459cbee9b13a81f8ddb718b579'
h=struct.unpack_from('<I',original,60)[0];opt=h+24;base=struct.unpack_from('<I',original,opt+28)[0];first=opt+struct.unpack_from('<H',original,h+20)[0]
def off(address):
 for i in range(struct.unpack_from('<H',original,h+6)[0]):
  s=first+40*i;start=base+struct.unpack_from('<I',original,s+12)[0];n=struct.unpack_from('<I',original,s+16)[0]
  if start<=address<start+n:return struct.unpack_from('<I',original,s+20)[0]+address-start
 raise ValueError('mutation outside file-backed data')
assert original[off(0x4451e2):off(0x4451e2)+2]==bytes.fromhex('7505');assert original[off(0x4451f8):off(0x4451f8)+2]==bytes.fromhex('ffd6')
@contextmanager
def modified():
 saved={p:p.read_bytes()for p in paths.values()}
 try:yield
 finally:
  for p,b in saved.items():p.write_bytes(b)
results=[]
def run(label,command=cmd,expect=True,error=None,record=True):
 started=time.perf_counter();r=subprocess.run(command,capture_output=True,text=True);elapsed=time.perf_counter()-started
 assert(r.returncode==0)==expect,(label,r.stderr)
 if error:assert error in r.stderr,(label,r.stderr)
 if record:results.append(dict(case=label,accepted=r.returncode==0,seconds=elapsed,diagnostic=r.stderr.strip()))
 return (json.loads(r.stdout)if r.returncode==0 else None),elapsed
positive,_=run('real_frozen_dispatcher_all147_resolved')
f=positive['functions'][name]['register_imports'];left=f['original'];right=f['rebuilt']
assert len(left['resolved_register_calls'])==147 and len(left['direct_import_calls'])==50 and len(right['direct_import_calls'])==197
assert not any(v['unresolved_register_calls']or v['other_unresolved_calls']for v in [left,right])
def inventory(side):return Counter(json.dumps(v,sort_keys=True)for key in ['resolved_register_calls','direct_import_calls']for v in side[key].values())
assert inventory(left)==inventory(right);assert len(inventory(left))==19
for label,address,data,error in [
 ('seed_bypass_new_predecessor',0x4451e2,bytes.fromhex('7514'),'unresolved register import calls'),
 ('wrong_register_first_call',0x4451f8,bytes.fromhex('ffd7'),'unresolved register import calls'),
 ('wrong_atoi_IAT_slot',0x4462e1,bytes.fromhex('8b1d3c114000'),'import graph differs'),
 ('partial_SI_clobber_live_cache',0x4451f2,bytes.fromhex('66be000090'),'unresolved register import calls'),
 ('partial_BH_clobber_live_cache',0x4462e7,bytes.fromhex('b7009090'),'unresolved register import calls'),
]:
 with modified():
  b=bytearray(original);b[off(address):off(address)+len(data)]=data;paths['original'].write_bytes(b);c=copy.deepcopy(config);c['target']['sha256']=hashlib.sha256(b).hexdigest();paths['config'].write_text(json.dumps(c));run(label,expect=False,error=error)
for label,keep,expected in [('existing_paths_need_no_direct_callee_contract',lambda c:'function'not in c['target'],True),('unknown_import_callee_barrier',lambda c:'import'not in c['target'],False)]:
 with modified():
  m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts']=[c for c in m['register_imports']['preservation_contracts']if keep(c)];paths['metadata'].write_text(json.dumps(m));run(label,expect=expected,error=None if expected else 'unresolved register import calls')
# Add a CALL to an already indexed/named ordinary helper inside a live ESI
# interval. Existing callee set stays unchanged; absent evidence must kill ESI.
with modified():
 b=bytearray(original);target=next(r['start']for r in config['driver']if r['name']=='_parse_switch@8');patch=b'\xe8'+struct.pack('<i',target-(0x4451f2+5))+b'\x90';b[off(0x4451f2):off(0x4451f2)+6]=patch;paths['original'].write_bytes(b);c=copy.deepcopy(config);c['target']['sha256']=hashlib.sha256(b).hexdigest();paths['config'].write_text(json.dumps(c));m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts']=[v for v in m['register_imports']['preservation_contracts']if 'function'not in v['target']];paths['metadata'].write_text(json.dumps(m));run('injected_unknown_direct_callee_in_live_cache',expect=False,error='unresolved register import calls')
# Two workloads: approved graph+RET/reference/extent checks, and that workload
# plus the NEW qualified-import/dataflow proof. They do not perform equal checks.
old=[str(baseline),*cmd[1:]];old.remove('--check-imports');old.remove('--resolve-register-imports');new_off=[cmd[0],*old[1:]]
old_report,_=run('approved_previous_workload',old);off_report,_=run('new_flag_off_exact_report',new_off);assert old_report==off_report
samples={'approved_graph_ABI_extent':[],'new_same_workload_flag_off':[],'new_plus_import_dataflow':[]}
commands=[old,new_off,cmd]
for c in commands:run('warmup',c,record=False)
for n in range(7):
 for key,c in zip(samples,commands):samples[key].append(run('timing',c,record=False)[1])
timings={k:dict(median_seconds=statistics.median(v),min_seconds=min(v),max_seconds=max(v),samples_seconds=v)for k,v in samples.items()}
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=hashlib.sha256(Path(cmd[0]).read_bytes()).hexdigest(),baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),input_sha256={k:hashlib.sha256(p.read_bytes()).hexdigest()for k,p in paths.items()},positive_report=positive,import_call_multiplicity=inventory(left),results=results,timings=timings),indent=2)+'\n')
print('Real LINK:147cached+50direct=197direct,19imports;7intended negatives+1scopecontrol; exact flag-off report;7-run timings passed.')
