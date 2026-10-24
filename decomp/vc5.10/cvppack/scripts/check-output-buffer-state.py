#!/usr/bin/env python3
"""Focused production-controller controls for the new buffer generation."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0104-output-buffer-initialization'
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
raw=module('buffer_raw',BINARY/'scripts/raw-state.py');controller=module('buffer_state',BINARY/'scripts/output-buffer-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((pair/'native/state.json').read_text())
def compare(a,b):return controller.validate(config,data,a,b)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason,changed_raw=None):
 a=copy.deepcopy(original) if changed_raw is None else raw.decode(changed_raw);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('missing-actual-attempt',lambda a,b:b['output_buffer_allocations'].clear(),'allocation attempt inventory')
reject('wrong-sequence',lambda a,b:b['output_buffer_allocations'][0].update(sequence=1),'allocation size/result')
reject('wrong-requested-capacity',lambda a,b:b['output_buffer_allocations'][0].update(size=65511),'allocation size/result')
reject('wrong-original-canonical-provider',lambda a,b:a['output_buffer_allocations'][0].update(provider=0),'canonical malloc/caller ABI')
reject('wrong-original-stack-cleanup',lambda a,b:a['output_buffer_allocations'][0].update(exit_esp=0),'canonical malloc/caller ABI')
reject('wrong-original-parent-return',lambda a,b:a['output_buffer_allocations'][0].update(parent_return=0),'canonical malloc/caller ABI')
reject('wrong-native-stack-cleanup',lambda a,b:b['output_buffer_allocations'][0].update(exit_esp=0),'native allocation/formal ABI')
reject('wrong-actual-file-formal',lambda a,b:b['output_buffer_allocations'][0]['formal_inventory'].update(extra='int'),'native allocation/formal ABI')
reject('wrong-file-input',lambda a,b:b['output_buffer_allocations'][0].update(file=14),'allocation input file')
reject('original-ESI-clobber',lambda a,b:a['output_buffer_allocations'][0]['after']['nonvolatile'].__setitem__(1,0),'original nonvolatile')
reject('native-EDI-clobber',lambda a,b:b['output_buffer_allocations'][0]['after']['nonvolatile'].__setitem__(2,0),'native nonvolatile')
reject('changed-native-postmalloc-x87-payload',lambda a,b:b['output_buffer_allocations'][0]['after']['fp'].update(st7='0x00000000000000000001'),'own raw x87')
def original_fp(a,b):
 env=a['output_buffer_state']['environment'];payload=bytearray(env['fp']);payload[-1]^=1;env['fp']=bytes(payload)
reject('changed-original-frontier-rawFP',original_fp,'own raw x87')
reject('changed-native-errno',lambda a,b:b['output_buffer_state']['environment'].update(errno=1),'native error identity/state')
reject('new-buffer-global-alias',lambda a,b:b['globals'].update(output_buffer=(int(b['output_buffer_state']['address'])+4).to_bytes(4,'little').hex()),'new generation/global alias')
reject('wrong-defined-capacity',lambda a,b:b['globals'].update(output_capacity=(65511).to_bytes(4,'little').hex()),'defined field')
reject('changed-initialized-header',lambda a,b:b['output_buffer_state'].update(bytes='00'+b['output_buffer_state']['bytes'][2:]),'complete initialized header')
reject('truncated-initialized-header',lambda a,b:b['output_buffer_state'].update(bytes=b['output_buffer_state']['bytes'][:-2]),'complete initialized header')
reject('unwritten-tail-misrepresented-as-defined',lambda a,b:b['output_buffer_state'].update(bytes=b['output_buffer_state']['bytes']+'00'),'complete initialized header')
reject('wrong-writer-position',lambda a,b:b['globals'].update(image_read_mode=(4953).to_bytes(4,'little').hex()),'writer position/used result')
reject('current-memory-crosses-new-generation-base',lambda a,b:b['memory'].update({str(b['output_buffer_state']['address']-4):'00'*8}),'typed generation namespace')
reject('current-string-crosses-new-generation-base',lambda a,b:b['strings'].update({str(b['output_buffer_state']['address']-4):'00'*8}),'typed generation namespace')
# Decode mutated raw observations, not fabricated receipt claims.
line=next(l for l in events.splitlines() if l.startswith('output-buffer-allocation-call '));fields=line.split();fields[3]='65511';changed=events.replace(line,' '.join(fields),1)
reject('raw-wrong-malloc-size',lambda a,b:None,'allocation size/result',changed)
line=next(l for l in events.splitlines() if l.startswith('output-buffer-state '));fields=line.split();fields[-1]='00'+fields[-1][2:];changed=events.replace(line,' '.join(fields),1)
reject('raw-changed-initialized-header',lambda a,b:None,'complete initialized header',changed)
result=dict(schema=1,native_only=True,scope='Exact production output-buffer-state.validate controls over raw-decoded current pair; full production accept check-only independently passes whole prefix.',fresh_negative_count=len(rejected),rejected=rejected,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/output-buffer-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0103-packed-directory-comparator-complete/empty-fp-review.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0103-packed-directory-comparator-complete/empty-fp-review.json'),scope='Prior32 sort/ABI/rawFP controls unchanged semantic scope; fresh104 whole-prefix pair reruns all375callbacks/full840B/errors/FP;24 focused new-generation/ABI/initialized8B/namespace/error/FP controls here.'))
(pair/'buffer-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
