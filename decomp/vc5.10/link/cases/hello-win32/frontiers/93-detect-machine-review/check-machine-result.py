#!/usr/bin/env python3
"""Exercise the extra seek/read, WORD result and exact two-byte producer store."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import time
from boundary_state import compare

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',required=True,type=Path)
parser.add_argument('--native',required=True,type=Path)
parser.add_argument('--report',required=True,type=Path)
args=parser.parse_args();frontier='_initialize_machine@4'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in('seek_file_calls','mapped_seek_calls'):
    negative('missing fifth '+key,lambda s,k=key:s[k].pop())
    for i in range(4):negative(key+' fifth formal%d'%i,lambda s,k=key,j=i:s[k][4]['formals'].__setitem__(j,0x12345678))
    for field,value in [('normal_return',False),('return_stack',1),('result',1),('stack_pointer',1),('hidden_inputs',['ecx']),('record_before','00'*40),('record_after','00'*40),('error_after','01000000')]:
        negative(key+' fifth '+field,lambda s,k=key,f=field,v=value:s[k][4].__setitem__(f,v))
for field,value in [('address',1),('bytes',2),('owner_local_address',1),('owner_local_bytes',8),('before','01000000'),('owner_kind','retired.error')]:
    negative('detector error owner '+field,lambda s,f=field,v=value:s['seek_file_calls'][4]['output'].__setitem__(f,v))
for i in range(3):negative('signature read formal%d'%i,lambda s,j=i:s['detect_read_entry']['formals'].__setitem__(j,0))
for field,value in [('stack_pointer',1),('hidden_inputs',['ecx'])]:
    negative('signature entry '+field,lambda s,f=field,v=value:s['detect_read_entry'].__setitem__(f,v))
for field,value in [('address',1),('bytes',2),('owner_local_address',1),('owner_local_bytes',8),('before','000000'),('initial_value_contract','incoming DWORD input')]:
    negative('signature owner '+field,lambda s,f=field,v=value:s['detect_output'].__setitem__(f,v))
negative('missing signature normal return',lambda s:s['detect_read_returns'].clear())
for field,value in [('normal_return',False),('return_stack',1),('result',3)]:
    negative('signature return '+field,lambda s,f=field,v=value:s['detect_read_returns'][0].__setitem__(f,v))
for field,value in [('after','4d010700'),('record_after','00'*40),('owner_local_address',1)]:
    negative('signature output '+field,lambda s,f=field,v=value:s['detect_read_returns'][0]['output'].__setitem__(f,v))
for key in('detect_machine_returns','input_machine_returns'):
    negative('missing '+key,lambda s,k=key:s[k].clear())
    for field,value in [('normal_return',False),('return_stack',1),('stack_pointer',1),('hidden_inputs',['ecx'])]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k][0].__setitem__(f,v))
negative('wrong meaningful AX',lambda s:s['detect_machine_returns'][0].__setitem__('result',0x14d))
negative('raw AX contradicts result',lambda s:s['detect_machine_returns'][0].__setitem__('raw_result',0x1234014d))
negative('preexisting machine in caller context',lambda s:s.__setitem__('input_machine_context_before',s['input_machine_context_before'][:96]+'4c01'+s['input_machine_context_before'][100:]))
negative('lost whole caller context result',lambda s:s['input_machine_returns'][0].__setitem__('context_after','00'*664))
def bad_store(state):
    raw=bytearray.fromhex(state['link_context']);struct.pack_into('<I',raw,48,0x1234014c)
    state['link_context']=raw.hex();state['input_machine_returns'][0]['context_after']=raw.hex()
negative('coherent DWORD overwrite of WORD and adjacent field',bad_store)
for field,value in [('stack_pointer',1),('formals',[0]),('hidden_inputs',['ecx'])]:
    negative('initializer input '+field,lambda s,f=field,v=value:s['initialize_machine_entry'].__setitem__(f,v))
negative('initializer missing actual input',lambda s:s['frontier_inputs'].__setitem__('formals',[]))
negative('initializer invented hidden input',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
# Defined result is AX, and the wrapper's own physical EAX is not a return
# value. Retain both raw words; these positive controls are not capture edits.
state=copy.deepcopy(native);state['detect_machine_returns'][0]['raw_result']=0xa1b2014c
state['input_machine_returns'][0]['raw_result']=0x11223344
compare(frontier,args.original,args.native,trap,state)
state=copy.deepcopy(native)
for output in(state['detect_output'],state['detect_read_returns'][0]['output']):output['before']='a1b2c3d4'
compare(frontier,args.original,args.native,trap,state)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/machine_result_state.py',root/'scripts/detect_machine_state.py',root/'scripts/input_machine_state.py',root/'scripts/mapped_seek_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,own_upper_word_and_void_eax_positive=True,own_uninitialized_signature_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_scope_positives=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Machine producer/consumer contract passes; %d affected negatives rejected.'%len(checks))
