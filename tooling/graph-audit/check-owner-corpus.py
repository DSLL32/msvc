#!/usr/bin/env python3
"""Frozen C2 full65 graph/import union with reader-only cached-import resolution.

Scratch static inputs only: no target execution, Wine, build or second CFG.
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
root=a.snapshot.resolve();assert root.is_relative_to(Path.home()/'.ai-tmp');cmd=json.loads((root/'command.json').read_text());paths={k:Path(cmd[cmd.index('--'+k)+1])for k in ['original','rebuilt','map','config','runtime','rebuilt-runtime','metadata']};assert all(v.resolve().is_relative_to(root)for v in paths.values());name='@read_initializer_records@4';metadata=json.loads(paths['metadata'].read_text());configuration=json.loads(paths['config'].read_text());original=paths['original'].read_bytes()
def sha(b):return hashlib.sha256(b).hexdigest()
h=struct.unpack_from('<I',original,60)[0];o=h+24;base=struct.unpack_from('<I',original,o+28)[0];first=o+struct.unpack_from('<H',original,h+20)[0]
def off(address):
 for i in range(struct.unpack_from('<H',original,h+6)[0]):
  q=first+40*i;start=base+struct.unpack_from('<I',original,q+12)[0];n=struct.unpack_from('<I',original,q+16)[0]
  if start<=address<start+n:return struct.unpack_from('<I',original,q+20)[0]+address-start
 raise ValueError('mutation not file-backed')
@contextmanager
def modified():
 saved={p:p.read_bytes()for p in paths.values()}
 try:yield
 finally:
  for p,b in saved.items():p.write_bytes(b)
results=[]
def run(label,command=cmd,expected=True,error=None,record=True):
 t=time.perf_counter();q=subprocess.run(command,capture_output=True,text=True);seconds=time.perf_counter()-t;assert(q.returncode==0)==expected,(label,q.stderr)
 if error:assert error in q.stderr,(label,error,q.stderr)
 x=json.loads(q.stdout)if not q.returncode else None
 if record:results.append(dict(case=label,accepted=not q.returncode,seconds=seconds,diagnostic=q.stderr.strip()))
 return x,seconds
expected_functions={r['name']for r in configuration['driver']} & set(configuration['accepted'])
positive,_=run('full65_one_graph_import_audit_reader_only')
assert positive['checked_functions']==65 and set(positive['functions'])==expected_functions
assert positive['register_import_inputs']['enabled_functions']==[name]
assert [n for n,f in positive['functions'].items()if'register_imports'in f]==[name]
r=positive['functions'][name]['register_imports'];assert r['require_all_register_calls_resolved']
assert r['original']['resolved_register_calls']=={str(0x474220):dict(dll='msvcrt.dll',name='free')};assert r['rebuilt']['direct_import_calls']=={str(0x402dae):dict(dll='msvcrt.dll',name='free')}
assert not any(v['unresolved_register_calls']or v['other_unresolved_calls']for v in[r['original'],r['rebuilt']])
# The prior64 retain byte-for-byte per-function reports; candidate matches the
# previously successful isolated strict resolver report, with all65 in ONE pass.
old=[str(a.baseline.resolve()),*cmd[1:]];old[old.index('--metadata')+1]=str(root/'metadata.json');old[old.index('--config')+1]=str(root/'configuration-prefix.json')
prefix,_=run('approved_prior64_direct_import_graph',old)
assert {k:v for k,v in positive['functions'].items()if k!=name}==prefix['functions']
reader=list(old);reader[reader.index('--config')+1]=str(root/'configuration-registers.json');reader.append('--resolve-register-imports');single,_=run('approved_reader1_global_resolver',reader);assert single['functions'][name]==positive['functions'][name]
assert set(prefix['functions'])|set(single['functions'])==set(positive['functions'])
for i,c in enumerate(metadata['register_imports']['preservation_contracts']):
 with modified():
  m=copy.deepcopy(metadata);m['register_imports']['preservation_contracts'].pop(i);paths['metadata'].write_text(json.dumps(m));run('remove '+json.dumps(c['target'],sort_keys=True),expected=False,error='unresolved register import calls')
for label,mutator,error in [
 ('drop_owner_enable',lambda m:m['functions'][name]['register_imports'].pop('enabled'),'import graph differs'),
 ('false_owner_enable',lambda m:m['functions'][name]['register_imports'].update(enabled=False),'enabled must be true'),
 ('stale_callee_evidence',lambda m:m['register_imports']['preservation_contracts'][0]['evidence'].update(sha256='0'*64),'stale preservation evidence'),
 ('typo_owner',lambda m:m['functions'].update({'@typo@4':m['functions'].pop(name)}),'not selected/accepted'),
 ('unselected_owner',lambda m:m['functions'].update({'@new_initializer@4':m['functions'].pop(name)}),'not selected/accepted'),
]:
 with modified():
  m=copy.deepcopy(metadata);mutator(m);paths['metadata'].write_text(json.dumps(m));run(label,expected=False,error=error)
with modified():
 c=copy.deepcopy(configuration);c['accepted'].remove(name);paths['config'].write_text(json.dumps(c));run('enabled_owner_removed_from_acceptance',expected=False,error='not selected/accepted')
# Adding global resolution still affects old functions; owner mode cannot
# secretly override it, guess stack-held pointers, or waive their differences.
run('global_still_checks_old_stack_holder_semantics',cmd+['--resolve-register-imports'],expected=False,error='_c2_initialize_options import graph differs')
with modified():
 m=copy.deepcopy(metadata);m['functions'].setdefault('_c2_initialize_options',{})['register_imports']={'enabled':True};paths['metadata'].write_text(json.dumps(m));run('enabling_old_owner_does_not_waive_mismatch',expected=False,error='_c2_initialize_options import graph differs')
for label,address,data,error in [
 ('reader_partial_BP_live_clobber',0x47420e,bytes.fromhex('66bd000090909090'),'unresolved register import calls'),
 ('reader_wrong_register_call',0x474220,bytes.fromhex('ffd6'),'unresolved register import calls'),
 ('reader_wrong_IAT_seed',0x4523dd,bytes.fromhex('8b2de4d04800'),'unresolved register import calls'),
]:
 with modified():
  b=bytearray(original);b[off(address):off(address)+len(data)]=data;paths['original'].write_bytes(b);c=copy.deepcopy(configuration);c['target']['sha256']=sha(b);paths['config'].write_text(json.dumps(c));run(label,expected=False,error=error)
with modified():
 b=bytearray(original);needle=b'SetConsoleCtrlHandler\0';assert b.count(needle)==1;at=b.index(needle);b[at]=ord('X');paths['original'].write_bytes(b);c=copy.deepcopy(configuration);c['target']['sha256']=sha(b);paths['config'].write_text(json.dumps(c));run('old64_import_identity_still_enforced',expected=False,error='_c2_main import graph differs')
# Removing a non-owner from accepted is legitimate tool subset analysis, but
# must be rejected by the full-case union/completeness assertion at the caller.
with modified():
 c=copy.deepcopy(configuration);c['accepted'].remove('_c2_main');paths['config'].write_text(json.dumps(c));x,_=run('scope_control_subset_needs_target_full_union_guard');assert x['checked_functions']==64 and set(x['functions'])!=expected_functions
# Startup, all65 graph/import/RET/extents and one-owner dataflow/provenance.
run('warmup',record=False);samples=[run('timing',record=False)[1]for i in range(7)]
a.report.write_text(json.dumps(dict(scope=__doc__,binary_sha256=sha(Path(cmd[0]).read_bytes()),baseline_sha256=sha(a.baseline.read_bytes()),inputs_sha256={k:sha(p.read_bytes())for k,p in paths.items()},positive_report=positive,old64_per_function_exact=True,reader1_per_function_exact=True,full65_union_exact=True,results=results,timing=dict(median_seconds=statistics.median(samples),min_seconds=min(samples),max_seconds=max(samples),samples_seconds=samples)),indent=2)+'\n');print(f'C2 owner-enable:{len(results)} controls/full65union/old64exact/reader1exact/7timings passed.')
