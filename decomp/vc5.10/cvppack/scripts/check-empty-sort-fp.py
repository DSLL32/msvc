#!/usr/bin/env python3
"""Scoped raw coherent-baseline controls for the empty-x87 projection."""
import copy
import difflib
import hashlib
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0103-packed-directory-comparator-complete'
spec=importlib.util.spec_from_file_location('empty_sort_raw',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native53/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/folder).glob(pattern)} for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native53','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
def environments(state):return list(state['sort_environments'].values())+[row[phase] for row in state['sort_calls'] for phase in ('before','after')]
def original_fp(offset,value):
 lines=[];count=0
 for line in events.splitlines():
  if line.startswith('sort-fp '):
   fields=line.split();raw=bytearray.fromhex(fields[2]);assert len(raw)==112
   raw[offset:offset+2]=value.to_bytes(2,'little');fields[2]=raw.hex();line=' '.join(fields);count+=1
  lines.append(line)
 assert count==752,count
 return '\n'.join(lines)+'\n'
# Recover and pin the exact controller under which the old28 controls ran.
# The executable delta is only one additional assertion and report fields.
current=(BINARY/'scripts/complete-sort-state.py').read_text()
old=current.replace(current[:current.index('import struct')], '"""Full canonical qsort callback/return/array/error/x87 evidence for this case."""\n')
old=old.replace("    assert control==0x027f and original_status==0 and original_tags==0xffff,'sort inert empty x87 projection requires exact observed027f/zero-status/all-empty tags'\n",'')
old=old.replace(",inert_empty_payload_projection_scope='hello-debug-dll integer sort through begin_buffered_output only; no reached FP-state consumer',all_eight_x87_tags_empty_throughout=True",'')
previous=json.loads((pair/'sort-checks.json').read_text())
assert hashlib.sha256(old.encode()).hexdigest()==previous['validator_sha256'],'old controller content/delta differs'
old_path=pair/'complete-sort-state-before-empty-guard.py';old_path.write_text(old)
delta=pair/'empty-fp-guard.diff';delta.write_text(''.join(difflib.unified_diff(old.splitlines(True),current.splitlines(True),fromfile='old-controller',tofile='current-controller')))
rejected=[]
def reject(name,state,raw,reason):
 try:compare(state,raw)
 except AssertionError as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
baseline=compare(native)
for name,offset,value,register in [('coherent-nonempty-st0-zero-tag',8,0xfffd,'ftag'),('coherent-nonzero-status',4,1,'fstat'),('coherent-wrong-control',0,0x037f,'fctrl')]:
 state=copy.deepcopy(native)
 for env in environments(state):env['fp'][register]='0x%08x'%value
 reject(name,state,original_fp(offset,value),'sort inert empty x87 projection requires exact observed027f/zero-status/all-empty tags')
state=copy.deepcopy(native);state['sort_environments']['after']['fp']['st7']='0x0123456789abcdef0123'
reject('changed-poststate-only-unused-ST7',state,events,'sort wrapper changed native error/x87 state')
state=copy.deepcopy(native)
for env in environments(state):env['fp']['st7']='0x0123456789abcdef0123'
compare(state)
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,consistent_own_empty_tag_ST7_positive=True,baseline=baseline['output_directory']['complete_sort'],seconds=time.perf_counter()-start,
 old_controller=dict(path=str(old_path.relative_to(BINARY)),sha256=sha(old_path)),monotonic_guard_delta=dict(path=str(delta.relative_to(BINARY)),sha256=sha(delta)),checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/complete-sort-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native53/receipt.json'),
 reused=dict(path=str((pair/'sort-checks.json').relative_to(BINARY)),sha256=sha(pair/'sort-checks.json'),scope='28 old controls reused under monotonic guard-only controller refinement: exact old controller069ae4 recovered/hash-verified; delta adds only exact empty-FP assertion/report fields and scope documentation, removes no old predicate. Captures/checker/target source unchanged. Four fresh semantic controls; no original/native execution.'))
(pair/'empty-fp-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
