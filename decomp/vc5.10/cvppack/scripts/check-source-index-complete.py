#!/usr/bin/env python3
"""Affected new-generation, temporary-free and output controls; native only."""
import copy
import importlib.util
import json
from pathlib import Path
import re
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0101-append-source-name-complete'
spec=importlib.util.spec_from_file_location('source_complete_raw',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native-abi/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/folder).glob(pattern)} for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native-abi','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);rejected=[]
def reject(name,mutate,reason,raw=events):
 state=copy.deepcopy(native);mutate(state)
 try:compare(state,raw)
 except (AssertionError,KeyError,IndexError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('wrong-generation-after-prior-frees',lambda s:(s['source_family_allocations'][1].update(after_frees=11),s['checked_allocations'][-2].update(after_frees=11)),'source family checked allocator generation ledger differs')
reject('wrong-appended-node-owner',lambda s:(s['source_family_allocations'][1].update(owner='build_source_file_index'),s['checked_allocations'][-2].update(owner='build_source_file_index')),'source family actual allocation owner differs')
reject('missing-new-output-generation',lambda s:s['source_family_allocations'].pop(),'source family checked allocator generation ledger differs')
reject('wrong-temp-free-argument',lambda s:s['source_family_frees'][1].update(address=s['source_family_frees'][1]['address']+4),'source family temporary free ownership/extent differs')
reject('wrong-temp-free-order',lambda s:s['source_family_frees'].reverse(),'source family temporary free call owner/order differs')
reject('temp-free-not-returned',lambda s:s['source_family_frees'][0].update(returned=False),'source family free cdecl argument/return differs')
reject('temp-free-wrong-stack-cleanup',lambda s:s['source_family_frees'][0].update(exit_esp=s['source_family_frees'][0]['exit_esp']+4),'source family free cdecl argument/return differs')
reject('temp-free-changed-error',lambda s:s['source_family_frees'][1].update(exit_last_error=1),'source family free LastError differs')
def change(state,index,offset):
 raw=bytearray.fromhex(state['source_family_frees'][index]['before']);raw[offset]^=1;state['source_family_frees'][index]['before']=raw.hex()
reject('changed-module-number',lambda s:change(s,0,0),'source family module header/offset initialized input differs')
reject('changed-file-offset',lambda s:change(s,0,8),'source family module header/offset initialized input differs')
reject('changed-name-source-pointer',lambda s:change(s,1,0),'source family name pointer/offset/link initialized input differs')
reject('changed-name-list-next',lambda s:change(s,1,8),'source family name pointer/offset/link initialized input differs')
reject('truncated-name-prefree-capacity',lambda s:s['source_family_frees'][1].update(before=s['source_family_frees'][1]['before'][:-2]),'source family complete prefree capacity missing')
reject('wrong-new-output-owner',lambda s:s['source_family_output'].update(address=s['source_family_output']['address']+4),'source family global output alias differs')
reject('truncated-live-output',lambda s:s['source_family_output'].update(bytes=s['source_family_output']['bytes'][:-2]),'source family output complete initialized extent missing')
def output_byte(state,offset):
 raw=bytearray.fromhex(state['source_family_output']['bytes']);raw[offset]^=1;state['source_family_output']['bytes']=raw.hex()
for offset,role in [(0,'header'),(74,'module-count'),(144,'file-offset'),(159,'counted-name')]:
 reject('changed-output-'+role,lambda s,o=offset:output_byte(s,o),'source family complete initialized output differs')
reject('wrong-append-actual-offset',lambda s:(s['source_family_allocations'][1]['arguments'].update(offset=1),s['checked_allocations'][-2]['arguments'].update(offset=1)),'source append actual pointer/scalar arguments differ')
reject('changed-append-sentinel-input',lambda s:(s['source_family_allocations'][1]['arguments'].update(tail_before='01'+'00'*15),s['checked_allocations'][-2]['arguments'].update(tail_before='01'+'00'*15)),'source append initialized sentinel input differs')
reject('wrong-append-returned-node',lambda s:(s['source_family_allocations'][2]['after_append'].update(name_tail=0),s['checked_allocations'][-1]['after_append'].update(name_tail=0)),'source append actual returned node/link aliases differ')
# Name reservedDWORD retains the actual native allocation's own unwritten bytes.
changed=copy.deepcopy(native);change(changed,1,12);compare(changed)
# Test whole interval exclusions, including crossing-before-base requests.
original=decoder.decode(events)
for index in (0,1):
 address=original['source_family_allocations'][index]['address']
 raw=events+'memory %08x %s\n'%(address-4,'00'*8)
 reject('original-crossing-retired-temp-'+str(index),lambda s:None,'source temporary retired payload present in current memory',raw)
 base=native['source_family_allocations'][index]['address']
 reject('native-crossing-retired-temp-'+str(index),lambda s,a=base:s['memory'].update({str(a-4):'00'*8}),'released payload was captured as current state' if any(row['address']<=base<row['address']+row['size'] for row in native['main_free_calls']) else 'source temporary retired payload present in current memory/string')
base=original['source_family_output']['address'];raw=events+'memory %08x %s\n'%(base+4,'00'*8)
reject('original-interior-typed-output-namespace',lambda s:None,'source family output must remain a typed generation namespace',raw)
# Raw returned-provider identity/control checks retain exact actual bytes. These
# feed the decoder directly instead of relying on an outer receipt hash failure.
line=next(line for line in events.splitlines() if line.startswith('source-family-free-call '));fields=line.split();fields[4]='00000000';raw=events.replace(line,' '.join(fields),1)
reject('raw-wrong-canonical-free-provider',lambda s:None,'source family actual canonical free provider differs',raw)
line=next(line for line in events.splitlines() if line.startswith('source-family-free-return '));raw=events.replace(line,line.replace(line.split()[1],'0040f3e9'),1)
reject('raw-wrong-free-return-sequence',lambda s:None,'source family free return order differs',raw)
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,own_unwritten_reserved_dword_positive=True,baseline=baseline['source_index_context'],seconds=time.perf_counter()-start,
 checker_sha256=sha(Path(__file__)),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),validator_sha256=sha(BINARY/'scripts/source-index-complete.py'),source_sha256=sha(BINARY/'src/source-index.c'),
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native-abi/receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0100-source-file-index/source-entry-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0100-source-file-index/source-entry-checks.json'),scope='Prior12free/type-table/firstcallback ABI/old-generation controls unchanged; new completed family/lifetimes/layout controls fresh here.'))
(pair/'source-complete-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
