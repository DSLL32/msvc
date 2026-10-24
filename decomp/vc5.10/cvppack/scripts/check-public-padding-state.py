#!/usr/bin/env python3
"""Focused initialized public-node, bump-owner, return and six-argument controls."""
import copy
import importlib.util
import json
from pathlib import Path
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0107-public-page-padding'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
raw=module('padding_raw','raw-state.py');padding=module('padding_state','public-padding-state.py');public=module('writer_state','public-writer-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((pair/'native/state.json').read_text())
# The whole production positive independently validates these allocation streams;
# use their same reached arenas here without reimplementing graph discovery.
aliases={};sizes={}
for kind in ('image_allocations','checked_allocations','cleared_allocations'):
 for a,b in zip(original[kind],native[kind]):aliases[a['address']]=b['address'];sizes[a['address']]=a['size']
def compare(a,b):public.validate(config,data,a,b);return padding.validate(config,data,a,b,aliases,sizes)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason,changed_raw=None):
 a=copy.deepcopy(original) if changed_raw is None else raw.decode(changed_raw);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('missing-before-input',lambda a,b:b['public_padding'].pop('reserve'),'reserve')
reject('wrong-actual-reserve',lambda a,b:b['public_padding'].update(reserve=0),'inputs/result')
reject('wrong-actual-return-result',lambda a,b:b['public_padding'].update(result=0),'inputs/result')
reject('wrong-original-entry-return',lambda a,b:a['public_padding'].update(return_pc=0),'inputs/result')
reject('wrong-original-return-stack',lambda a,b:a['public_padding'].update(exit_esp=0),'cdecl return stack')
reject('wrong-native-return-stack',lambda a,b:b['public_padding'].update(exit_esp=0),'cdecl return stack')
reject('clobbered-original-EDI',lambda a,b:a['public_padding']['after_nonvolatile'].__setitem__(2,a['public_padding']['after_nonvolatile'][2]^1),'ordinary return registers')
reject('clobbered-native-EBP',lambda a,b:b['public_padding']['after_nonvolatile'].__setitem__(3,b['public_padding']['after_nonvolatile'][3]^1),'ordinary return registers')
reject('wrong-bucket-owner',lambda a,b:b['public_padding'].update(buckets_address=0),'bucket owner')
reject('truncated-before-bucket-capacity',lambda a,b:b['public_padding'].update(buckets=b['public_padding']['buckets'][:-2]),'input bucket extent')
reject('missing-initialized-input-node',lambda a,b:b['public_padding']['nodes'].pop(),'node inventory')
reject('wrong-input-node-bucket',lambda a,b:b['public_padding']['nodes'][0].update(bucket=4095),'ownership/order')
reject('wrong-input-node-owner',lambda a,b:b['public_padding']['nodes'][0].update(address=b['public_padding']['nodes'][0]['address']+4),'ownership/order')
reject('truncated-initialized-input-record',lambda a,b:b['public_padding']['nodes'][0].update(bytes=b['public_padding']['nodes'][0]['bytes'][:-2]),'node input extent')
reject('changed-initialized-input-record',lambda a,b:b['public_padding']['nodes'][0].update(bytes=b['public_padding']['nodes'][0]['bytes'][:10]+'ff'+b['public_padding']['nodes'][0]['bytes'][12:]),'input record/hash')
reject('wrong-input-chain',lambda a,b:b['public_padding'].update(buckets='00'*16384),'before bucket chain')
reject('wrong-bump-remaining',lambda a,b:b['public_padding'].update(remaining=b['public_padding']['remaining']-1),'input bump cursor/remaining')
reject('wrong-bump-cursor',lambda a,b:b['public_padding'].update(cursor=b['public_padding']['cursor']+4),'input bump cursor/remaining')
reject('changed-bump-result-global',lambda a,b:b['globals'].update(public_arena_remaining='00000000'),'bump allocation effect')
reject('wrong-new-cursor-global',lambda a,b:b['globals'].update(public_arena_cursor='00000000'),'bump allocation effect')
def marker_change(a,b):
 address=b['public_padding']['cursor'];value=bytearray.fromhex(b['memory'][str(address)]);value[-1]^=1;b['memory'][str(address)]=value.hex()
reject('changed-sentinel-payload',marker_change,'end-marker record')
reject('truncated-new-initialized-node',lambda a,b:b['memory'].__setitem__(str(b['public_padding']['cursor']),b['memory'][str(b['public_padding']['cursor'])][:-2]),'end-marker record')
def old_node_change(a,b):
 address=b['public_padding']['nodes'][0]['address'];value=bytearray.fromhex(b['memory'][str(address)]);value[-1]^=1;b['memory'][str(address)]=value.hex()
reject('changed-existing-post-node',old_node_change,'existing initialized node')
def current_table(a,b):
 address=b['public_padding']['buckets_address'];value=bytearray.fromhex(b['memory'][str(address)]);value[-4:]=bytes(4);b['memory'][str(address)]=value.hex()
reject('missing-last-bucket-end-marker',current_table,'bucket mutation')
reject('changed-new-byte-count',lambda a,b:b['globals'].update(public_symbol_bytes='00000000'),'byte-count consumption')
reject('wrong-namehash-callback',lambda a,b:b['parameters']['key'].update(function='compare_packed_directory_entries'),'namehash argument roles')
reject('wrong-namehash-WORD-field',lambda a,b:b['parameters']['method'].update(field='address_method'),'namehash argument roles')
reject('wrong-namehash-output-WORD-width',lambda a,b:b['parameters']['method'].update(width=4),'namehash argument roles')
reject('wrong-namehash-DWORD-field',lambda a,b:b['parameters']['bytes'].update(field='symbol_bytes'),'namehash argument roles')
reject('wrong-namehash-stream-owner',lambda a,b:b['parameters']['stream'].update(local='addresses'),'namehash argument roles')
reject('claimed-uninitialized-stream-value',lambda a,b:b['parameters']['stream'].update(bytes='00'*24),'namehash argument roles')
reject('changed-parent-current-header',lambda a,b:b['public_writer_frame'].update(header='00'*16),'initialized header/padding')
line=next(line for line in events.splitlines() if line.startswith('public-padding-node '));fields=line.split();fields[-1]=fields[-1][:10]+'ff'+fields[-1][12:]
reject('raw-changed-before-record-input',lambda a,b:None,'input record/hash',events.replace(line,' '.join(fields),1))
# Future stream builder owns this uninitialized object; no pre-call byte value
# is prescribed by the writer or the padding routine.
a=copy.deepcopy(original);contents=bytearray(a['memory'][a['esp']]);contents[64:112]=bytes(range(48));a['memory'][a['esp']]=bytes(contents);assert compare(a,native)==baseline
result=dict(schema=1,native_only=True,scope='Exact production public-writer+padding validators over decoded actual pair; whole production positive independently checks all allocators, formal6 ABI, prior375callbacks/58copies/current lifetime namespaces. No implementation-mirroring synthetic execution, no new original for controls.',fresh_negative_count=len(rejected),rejected=rejected,positive_own_uninitialized_stream_bits=True,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),validator_sha256=sha(BINARY/'scripts/public-padding-state.py'),public_writer_validator_sha256=sha(BINARY/'scripts/public-writer-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0106-write-publics-prefix/public-writer-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0106-write-publics-prefix/public-writer-checks.json'),scope='Historical32 public prologue controls and prior24copy/24allocation/32sort controls retain their original scopes. Fresh controls here cover new input/output/node/bump/WORD-DWORDfield/namehash schema; actual full fresh prefix repeats previous behavior.'))
(pair/'padding-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
