#!/usr/bin/env python3
"""Exercise the live DWORD output and full-width pending swap input."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import time
from boundary_state import compare

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',required=True,type=Path)
parser.add_argument('--native',required=True,type=Path)
parser.add_argument('--report',required=True,type=Path)
args=parser.parse_args();frontier='_byte_swap_dword@4'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap']
native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)

for i in range(3):
    negative('mapped DWORD physical formal%d'%i,lambda s,j=i:s['big_endian_read_entry']['formals'].__setitem__(j,0x12345678))
for field,value in [('stack_pointer',1),('hidden_inputs',['ecx']),('formals',[])]:
    negative('mapped DWORD entry '+field,lambda s,f=field,v=value:s['big_endian_read_entry'].__setitem__(f,v))
for field,value in [('address',1),('bytes',3),('owner_local_address',1),('owner_local_bytes',8),('before','000000'),('initial_value_contract','incoming DWORD input')]:
    negative('DWORD output '+field,lambda s,f=field,v=value:s['big_endian_output'].__setitem__(f,v))
negative('missing mapped DWORD return',lambda s:s['big_endian_read_returns'].clear())
negative('duplicate mapped DWORD return',lambda s:s['big_endian_read_returns'].append(copy.deepcopy(s['big_endian_read_returns'][0])))
for field,value in [('normal_return',False),('return_stack',1),('result',3),('stack_pointer',1),('hidden_inputs',['ecx']),('formals',[])]:
    negative('mapped DWORD returned '+field,lambda s,f=field,v=value:s['big_endian_read_returns'][0].__setitem__(f,v))
for field,value in [('after','4c010701'),('record_after','00'*40),('address',1),('owner_local_address',1),('owner_local_bytes',8),('before','000000')]:
    negative('mapped DWORD returned output '+field,lambda s,f=field,v=value:s['big_endian_read_returns'][0]['output'].__setitem__(f,v))
for field,value in [('stack_pointer',1),('formals',[]),('formals',[0x4c010700]),('hidden_inputs',['ecx'])]:
    negative('swap DWORD entry '+field+str(value),lambda s,f=field,v=value:s['byte_swap_entry'].__setitem__(f,v))
negative('swap missing physical input',lambda s:s['frontier_inputs'].__setitem__('formals',[]))
negative('swap invented hidden input',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
def copied_output(state):
    state['big_endian_read_entry']['formals'][1]-=4
    state['big_endian_read_returns'][0]['formals'][1]-=4
    for output in(state['big_endian_output'],state['big_endian_read_returns'][0]['output']):
        output['address']-=4;output['owner_local_address']-=4
negative('coherent copied DWORD local',copied_output)

# Incoming bytes of this new local are not inputs. A semantic-only changed
# own snapshot must still pass; actual file hashes remain capture-bound.
state=copy.deepcopy(native)
for output in(state['big_endian_output'],state['big_endian_read_returns'][0]['output']):output['before']='a1b2c3d4'
compare(frontier,args.original,args.native,trap,state)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/big_endian_state.py',root/'scripts/mapped_seek_state.py',root/'scripts/archive_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,uninitialized_own_bytes_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_uninitialized_positive=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('DWORD boundary passes; %d focused negatives rejected; arbitrary own incoming bytes accepted.'%len(checks))
