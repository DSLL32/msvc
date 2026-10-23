#!/usr/bin/env python3
"""Counted class-name ownership and narrow kind/output-pointer ABI controls."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
from binary import PE
from state import BINARY, ROOT, sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0071-match-forward-type'
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text())
events=(pair/'original/events').read_text();native=json.loads((pair/'native/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/side).glob(pattern)} for side,pattern in [('original','events-view-*.bin'),('native','native-view-*.bin')]}
spec=importlib.util.spec_from_file_location('raw_state',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);assert baseline['parameters']['kind']==0x1005 and baseline['parameters']['name']==dict(segment=0,offset=342,record_indices=[6],bytes='0e7461675041494e54535452554354')
rejected=[]
def reject(name,mutate,reason,raw=events):
 changed=copy.deepcopy(native);mutate(changed)
 try:compare(changed,raw)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),('wrong rejection guard',name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
for field,value in [('segment',1),('offset',343),('record_indices',[7]),('bytes','0e'+'00'*14)]:
 reject('wrong-counted-name-'+field,lambda s,f=field,v=value:s['parameters']['name'].update({f:v}),'typed ABI arguments differ')
reject('wrong-kind-low-word',lambda s:s['parameters'].update(kind=0x1004),'typed ABI arguments differ')
reject('wrong-name-real-pointee-width',lambda s:s['observed_argument_types'][1].update(pointee_width=4),'actual frontier pointee extent differs')
reject('wrong-output-local',lambda s:s['parameters']['match'].update(local='target'),'typed ABI arguments differ')
original=decoder.decode(events);lines=events.splitlines();index=max(i for i,line in enumerate(lines) if line.startswith('memory %08x '%original['esp']));prefix,payload=lines[index].rsplit(' ',1)
def stackword(slot,value):
 changed=list(lines);words=bytearray.fromhex(payload);words[slot*4:slot*4+4]=value.to_bytes(4,'little');changed[index]=prefix+' '+words.hex();return '\n'.join(changed)+'\n'
compare(native,stackword(1,original['stack'][1]^0x12340000))
reject('wrong-original-kind-low-word',lambda s:None,'typed ABI arguments differ',stackword(1,original['stack'][1]^1))
reject('wrong-original-name-interior',lambda s:None,'type name has no observed immediate-size local-record owner',stackword(2,original['stack'][2]+1))
reject('wrong-original-output-owner',lambda s:None,'original output argument caller-local identity differs',stackword(3,original['stack'][3]+4))
provider=PE(ROOT/config['target']['path']);assert provider.read(0x40ceb2,5)==bytes.fromhex('668b742414')
actual=[]
for field in ['kind-width','name-pointee']:
 with tempfile.TemporaryDirectory(prefix='cvpack-type-name-abi-',dir=Path.home()/'.ai-tmp') as temporary:
  directory=Path(temporary);changed=copy.deepcopy(config);row=next(r for r in changed['functions'] if r['name']=='lookup_named_type')
  if field=='kind-width':row['parameters'][0]['native_width']=4;reason='actual frontier formal width differs'
  else:row['parameters'][1]['native_pointee_width']=4;reason='actual pointer formal pointee extent differs'
  bad=directory/'configuration.json';bad.write_text(json.dumps(changed));output=directory/'native'
  result=subprocess.run([sys.executable,str(BINARY/'scripts/native-debug.py'),'--original',str(pair/'original'),'--config',str(bad),'--content-store',str(BINARY/'build/source-content'),'--output',str(output)],cwd=BINARY,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=15)
  diagnostic='\n'.join((output/file).read_text() for file in ('gdb-stdout','gdb-stderr'));assert result.returncode and reason in diagnostic,diagnostic
  actual.append(dict(name=field,returncode=result.returncode,diagnostic=diagnostic,bad_config_sha256=sha(bad),observer_sha256=sha(output/'gdb-state.py')))
report=dict(schema=1,native_only=True,count=len(rejected)+len(actual),rejected=rejected,actual_gdb_rejections=actual,parameters=baseline['parameters'],dead_kind_high_word_positive=True,original_kind_word_load=dict(address=0x40ceb2,bytes='668b742414'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),seconds=time.perf_counter()-start)
(pair/'type-name-state-checks.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(rejected=report['count'],seconds=report['seconds'])))
