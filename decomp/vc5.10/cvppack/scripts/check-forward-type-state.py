#!/usr/bin/env python3
"""Owned forward-type record, output slot and actual formal inventory controls."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from state import BINARY, sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0070-rewrite-bp-relative-symbol-typed-forward'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']==dict(record=dict(root='type_records',index=6),match=dict(owner='compact_type_index',local='target',width=4),insert=0)
rejected=[]
def reject(name,mutate,reason,raw=events):
 changed=copy.deepcopy(native);mutate(changed)
 try:compare(changed,raw)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-record-index',lambda s:s['parameters']['record'].update(index=7),'typed ABI arguments differ')
for field,value in [('owner','other'),('local','other'),('width',8)]:
 reject('wrong-output-'+field,lambda s,f=field,v=value:s['parameters']['match'].update({f:v}),'typed ABI arguments differ')
reject('wrong-insert-mode',lambda s:s['parameters'].update(insert=1),'typed ABI arguments differ')
reject('wrong-real-record-extent',lambda s:s['observed_argument_types'][0].update(pointee_width=20),'actual frontier pointee extent differs')
lines=events.splitlines();index=max(i for i,line in enumerate(lines) if line.startswith('registers '));fields=lines[index].split();fields[4]='%08x'%(int(fields[4],16)+4);lines[index]=' '.join(fields)
reject('wrong-original-output-slot',lambda s:None,'original output argument caller-local identity differs','\n'.join(lines)+'\n')
actual=[]
for label in ['missing-ABI-schema','wrong-output-pointee-extent']:
 with tempfile.TemporaryDirectory(prefix='cvpack-forward-abi-',dir=Path.home()/'.ai-tmp') as temporary:
  directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='match_forward_type')
  if label=='missing-ABI-schema':row['parameters']=[];reason='incomplete frontier ABI contract'
  else:row['parameters'][1]['width']=8;reason='actual pointer formal pointee extent differs'
  bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
  result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
  diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
  actual.append(dict(name=label,returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=baseline['parameters'],original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'forward-type-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
