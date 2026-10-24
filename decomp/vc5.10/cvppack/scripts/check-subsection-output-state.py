#!/usr/bin/env python3
"""Focused actual-call/source/relative-row/output controls for the copier."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import time
from state import BINARY,sha

start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0105-write-subsections'
def module(name,file):
 spec=importlib.util.spec_from_file_location(name,BINARY/'scripts'/file);result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
raw=module('sub_raw','raw-state.py');buffer=module('sub_buffer','output-buffer-state.py');sub=module('sub_state','subsection-output-state.py')
config=json.loads((pair/'original/configuration.json').read_text());data=json.loads((pair/'original/data.json').read_text());events=(pair/'original/events').read_text();original=raw.decode(events);native=json.loads((pair/'native/state.json').read_text())
def compare(a,b):buffer.validate(config,data,a,b);return sub.validate(config,data,a,b)
baseline=compare(original,native);rejected=[]
def reject(name,mutate,reason,changed_raw=None):
 a=copy.deepcopy(original) if changed_raw is None else raw.decode(changed_raw);b=copy.deepcopy(native);mutate(a,b)
 try:compare(a,b)
 except (AssertionError,KeyError) as error:
  assert reason in str(error),(name,str(error));rejected.append(dict(name=name,diagnostic=str(error)))
 else:raise AssertionError('accepted '+name)
reject('missing-last-call',lambda a,b:b['subsection_calls'].pop(),'complete call inventory')
reject('wrong-index-order',lambda a,b:b['subsection_calls'][1].update(index=0),'index/caller sequence')
reject('wrong-original-return',lambda a,b:a['subsection_calls'][0].update(return_pc_after=0),'index/caller sequence')
reject('wrong-native-stack-cleanup',lambda a,b:b['subsection_calls'][0].update(exit_esp=0),'cdecl/formal ABI')
reject('wrong-actual-formals',lambda a,b:b['subsection_calls'][0]['formal_inventory'].update(extra='int'),'cdecl/formal ABI')
reject('original-nonvolatile-clobber',lambda a,b:a['subsection_calls'][0]['after_nonvolatile'].__setitem__(1,a['subsection_calls'][0]['after_nonvolatile'][1]^1),'return registers')
reject('native-nonvolatile-clobber',lambda a,b:b['subsection_calls'][0]['after_nonvolatile'].__setitem__(2,b['subsection_calls'][0]['after_nonvolatile'][2]^1),'return registers')
reject('wrong-source-pointer',lambda a,b:b['subsection_calls'][0].update(input_address=b['subsection_calls'][0]['input_address']+4),'source pointer/row alias')
reject('changed-initialized-source',lambda a,b:b['subsection_calls'][0].update(input='ff'+b['subsection_calls'][0]['input'][2:]),'initialized source input')
reject('truncated-initialized-source',lambda a,b:b['subsection_calls'][0].update(input=b['subsection_calls'][0]['input'][:-2]),'initialized source input')
reject('wrong-copy-start-position',lambda a,b:b['subsection_calls'][1].update(before_used=0),'append position')
reject('wrong-padding-count',lambda a,b:b['subsection_calls'][0].update(padding=1),'padding/output count')
reject('wrong-postcopy-count',lambda a,b:b['subsection_calls'][0].update(after_used=0),'padding/output count')
reject('wrong-rewritten-offset',lambda a,b:b['subsection_calls'][0].update(after_row='00'*12),'relative row')
def current_row(a,b):
 base=int.from_bytes(bytes.fromhex(b['globals']['directory_entries']),'little');contents=bytearray.fromhex(b['memory'][str(base)]);contents[4]^=1;b['memory'][str(base)]=contents.hex()
reject('changed-current-relative-directory-row',current_row,'current directory offsets/tail')
def current_tail(a,b):
 base=int.from_bytes(bytes.fromhex(b['globals']['directory_entries']),'little');contents=bytearray.fromhex(b['memory'][str(base)]);contents[700]^=1;b['memory'][str(base)]=contents.hex()
reject('changed-current-initialized-inactive-tail',current_tail,'current directory offsets/tail')
reject('changed-output-prefix-copy',lambda a,b:b['output_buffer_state'].update(bytes=b['output_buffer_state']['bytes'][:20]+'ff'+b['output_buffer_state']['bytes'][22:]),'initialized output buffer')
reject('captured-unwritten-spare-as-defined',lambda a,b:b['output_buffer_state'].update(bytes=b['output_buffer_state']['bytes']+'00'),'initialized header')
reject('wrong-output-entry-index',lambda a,b:b['parameters']['entry'].update(index=57),'output-pointer/scalar ABI')
reject('wrong-output-entry-extent',lambda a,b:b['parameters']['entry'].update(extent=8),'output-pointer/scalar ABI')
reject('changed-final-ownFPpayload',lambda a,b:b['output_buffer_state']['environment']['fp'].update(st7='0x00000000000000000001'),'own raw x87')
reject('changed-final-LastError',lambda a,b:b['output_buffer_state']['environment'].update(last_error=1),'native error identity/state')
def stale_source(a,b):
 address=a['main_free_calls'][11]['address'];row=bytearray(a['directory_sort_complete']['bytes']);struct.pack_into('<I',row,4,address);a['directory_sort_complete']['bytes']=bytes(row);call=a['subsection_calls'][0];call['before_row']=bytes(row[:12]);call['input_address']=address
reject('coherent-retired-source-pointer-role',stale_source,'source overlaps a retired generation')
line=next(l for l in events.splitlines() if l.startswith('subsection-input '));fields=line.split();fields[-1]='ff'+fields[-1][2:];changed=events.replace(line,' '.join(fields),1)
reject('raw-changed-source-input',lambda a,b:None,'initialized source input',changed)
result=dict(schema=1,native_only=True,scope='Exact production buffer+subsection validators over raw-decoded current pair; full production acceptance independently checks entire prefix, actual GDB output-pointer12B schema and all375sort callbacks.',fresh_negative_count=len(rejected),rejected=rejected,baseline=baseline,seconds=time.perf_counter()-start,checker_sha256=sha(Path(__file__)),subsection_validator_sha256=sha(BINARY/'scripts/subsection-output-state.py'),buffer_validator_sha256=sha(BINARY/'scripts/output-buffer-state.py'),decoder_sha256=sha(BINARY/'scripts/raw-state.py'),original_receipt_sha256=sha(pair/'original/receipt.json'),native_receipt_sha256=sha(pair/'native/receipt.json'),reused=dict(path='cases/hello-debug-dll/evidence/0104-output-buffer-initialization/buffer-checks.json',sha256=sha(BINARY/'cases/hello-debug-dll/evidence/0104-output-buffer-initialization/buffer-checks.json'),scope='Prior24 allocation-generation/ABI/namespace controls and earlier32 sort/FP controls retain their original case scope. Current whole-prefix fresh pair reruns actual allocation/375sorts;24 fresh copier/current-vs-history/output-pointer/source/FP controls here.'))
(pair/'subsection-checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(rejected=len(rejected),seconds=result['seconds'])))
