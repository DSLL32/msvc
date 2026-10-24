#!/usr/bin/env python3
"""Actual full-sort callback/ABI/error/x87 and completed-array controls."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0103-packed-directory-comparator-complete'
spec=importlib.util.spec_from_file_location('sort_raw',BINARY/'scripts/raw-state.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();native=json.loads((pair/'native53/state.json').read_text())
artifacts={'hello.dll':(pair/'original/artifact-hello.dll').read_bytes()};views={side:{p.name:p.read_bytes() for p in (pair/folder).glob(pattern)} for side,folder,pattern in [('original','original','events-view-*.bin'),('native','native53','native-view-*.bin')]}
def compare(state,raw=events):return decoder.compare(config,data,raw,state,artifacts,views)
baseline=compare(native);rejected=[]
def reject(name,mutate,reason,raw=events):
 state=copy.deepcopy(native);mutate(state)
 try:compare(state,raw)
 except (AssertionError,KeyError,IndexError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('missing-last-callback',lambda s:s['sort_calls'].pop(),'complete sort callback inventory differs')
reject('wrong-callback-sequence',lambda s:s['sort_calls'][1].update(sequence=0),'sort actual callback sequence/provider site differs')
reject('wrong-comparator-signed-result',lambda s:s['sort_calls'][0].update(result=1),'sort actual comparator signed return differs')
reject('wrong-callback-row-index',lambda s:s['sort_calls'][0].update(first=s['sort_calls'][0]['first']+12),'sort actual callback argument row/owner differs')
def rowbyte(state):
 raw=bytearray.fromhex(state['sort_calls'][0]['records']);raw[4]^=1;state['sort_calls'][0]['records']=raw.hex()
reject('changed-callback-live-data-owner',rowbyte,'sort actual callback typed row input differs')
reject('wrong-callback-ret-cleanup',lambda s:s['sort_calls'][0].update(exit_esp=s['sort_calls'][0]['exit_esp']+4),'sort actual callback cdecl stack/return differs')
reject('wrong-native-formal-inventory',lambda s:s['sort_calls'][0]['formal_inventory'].update(third='unsigned int'),'sort actual callback formal inventory differs')
reject('native-callee-EDI-clobber',lambda s:s['sort_calls'][0]['after']['nonvolatile'].__setitem__(2,0),'sort callback normal-return registers differ')
reject('native-callback-errno-change',lambda s:s['sort_calls'][0]['after'].update(errno=22),'sort callback changed native error/x87 state')
reject('native-wrapper-doserrno-change',lambda s:s['sort_environments']['after'].update(doserrno=5),'sort wrapper changed native error/x87 state')
reject('native-callback-LastError-change',lambda s:s['sort_calls'][0]['after'].update(last_error=5),'sort callback changed native error/x87 state')
reject('native-error-storage-identity-change',lambda s:s['sort_calls'][0]['before'].update(errno_address=s['sort_calls'][0]['before']['errno_address']+4),'sort callback changed native error/x87 state')
reject('missing-native-x87-register',lambda s:s['sort_calls'][0]['before']['fp'].pop('st7'),'incomplete native sort x87/register schema')
reject('truncated-native-x87-register',lambda s:s['sort_calls'][0]['before']['fp'].update(st7='0x00'),'native raw x87 register width/encoding differs')
reject('changed-native-x87-status',lambda s:s['sort_calls'][0]['after']['fp'].update(fstat='0x00000001'),'sort callback changed native error/x87 state')
reject('changed-native-final-x87-register',lambda s:s['sort_environments']['after']['fp'].update(st0='0x00000000000000000001'),'sort wrapper changed native error/x87 state')
reject('wrong-qsort-wrapper-stack',lambda s:s['directory_sort_complete'].update(exit_esp=0),'sort native wrapper cdecl return differs')
reject('truncated-completed-array',lambda s:s['directory_sort_complete'].update(bytes=s['directory_sort_complete']['bytes'][:-2]),'sort completed initialized array/current identity differs')
def all_env(state):
 return list(state['sort_environments'].values())+[row[phase] for row in state['sort_calls'] for phase in ('before','after')]
def control(state):
 for env in all_env(state):env['fp']['fctrl']='0x0000037f'
reject('preserved-but-wrong-native-startup-precision',control,'native startup x87 control differs from canonical CRT baseline')
def tags(state):
 for env in all_env(state):env['fp']['ftag']='0x00000000'
reject('preserved-but-wrong-native-startup-tags',tags,'native startup x87 status/tags differ from canonical CRT baseline')
# Unused x87 payload remains each process's own bits, with empty tags and full
# preservation. It is never seeded from Windows or normalized away.
changed=copy.deepcopy(native)
for env in all_env(changed):env['fp']['st7']='0x0123456789abcdef0123'
compare(changed)
# Raw semantic failures: no outer receipt/hash rejection substitutes here.
line=next(line for line in events.splitlines() if line.startswith('sort-crt-abi '));fields=line.split();payload=bytearray.fromhex(fields[2]);payload[0]^=1;fields[2]=payload.hex();raw=events.replace(line,' '.join(fields),1)
reject('raw-wrong-canonical-errno-instructions',lambda s:None,'sort canonical CRT error-access ABI differs',raw)
line=next(line for line in events.splitlines() if line.startswith('sort-errors return '));fields=line.split();payload=bytearray.fromhex(fields[-1]);payload[24]^=1;fields[-1]=payload.hex();raw=events.replace(line,' '.join(fields),1)
reject('raw-original-callback-errno-change',lambda s:None,'sort callback changed original error identity/state',raw)
fields=line.split();fields[4]=str(int(fields[4])+1);raw=events.replace(line,' '.join(fields),1)
reject('raw-original-callback-TLS-index-change',lambda s:None,'sort callback changed original error identity/state',raw)
line=next(line for line in events.splitlines() if line.startswith('sort-fp return '));fields=line.split();fields[2]=fields[2][:-2];raw=events.replace(line,' '.join(fields),1)
reject('raw-truncated-original-FloatSave',lambda s:None,'incomplete original sort error/FP/register state',raw)
line=next(line for line in events.splitlines() if line.startswith('sort-registers return '));fields=line.split();fields[3]='00000000';raw=events.replace(line,' '.join(fields),1)
reject('raw-original-callee-ESI-clobber',lambda s:None,'sort callback normal-return registers differ',raw)
line=next(line for line in events.splitlines() if line.startswith('sort-callback-return 0 '));fields=line.split();fields[3]='78020000';raw=events.replace(line,' '.join(fields),1)
reject('raw-original-wrong-return-address',lambda s:None,'sort actual callback cdecl stack/return differs',raw)
line=next(line for line in events.splitlines() if line.startswith('sort-wrapper-after '));fields=line.split();fields[2]='374';raw=events.replace(line,' '.join(fields),1)
reject('raw-original-incomplete-wrapper-count',lambda s:None,'sort original wrapper call/stack/count differs',raw)
# Controller inventories cannot self-confirm absent actual raw state.
line=next(line for line in events.splitlines() if line.startswith('sort-fp entry '));raw=events.replace(line+'\n','',1)
reject('raw-missing-per-callback-FP-state',lambda s:None,'fp',raw)
result=dict(schema=1,native_only=True,fresh_negative_count=len(rejected),rejected=rejected,baseline=baseline['output_directory']['complete_sort'],own_empty_tag_unused_x87_payload_positive=True,seconds=time.perf_counter()-start,
 original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native53/receipt.json'),checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/complete-sort-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),source_sha256=sha(BINARY/'src/writer.c'),observer_sha256=sha(BINARY/'scripts/sort-observer.inc'),
 reused=dict(path='cases/hello-debug-dll/evidence/0102-packed-file-writer-sort/directory-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0102-packed-file-writer-sort/directory-checks.json'),scope='21 input-history/owned12Barray/permutation/firstcallback/DWARF/import-site controls retained;28 fresh fullsort/returns/errors/TLS/nonvol/x87/completion controls here.'))
(pair/'sort-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
