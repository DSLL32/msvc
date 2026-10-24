#!/usr/bin/env python3
"""Reject damaged repeated-seek returns, error-local aliases and reader inputs."""
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
parser.add_argument('--frontier',default='_read_big_endian_dword@4',choices=['_read_big_endian_dword@4','_byte_swap_dword@4'])
args=parser.parse_args();frontier=args.frontier
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native);positive=time.perf_counter()-start
negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in('seek_file_calls','mapped_seek_calls'):
    negative('missing '+key,lambda s,k=key:s[k].pop())
    negative('reordered '+key,lambda s,k=key:s[k].reverse())
    for i in range(4):
        for j in range(4):
            negative('%s %d formal%d'%(key,i,j),lambda s,k=key,a=i,b=j:s[k][a]['formals'].__setitem__(b,0x12345678))
        for field,value in [('normal_return',False),('return_stack',1),('result',7),('record_before','00'*40),('record_after','00'*40),('error_after','01000000'),('stack_pointer',1),('hidden_inputs',['ecx'])]:
            negative('%s %d %s'%(key,i,field),lambda s,k=key,a=i,f=field,v=value:s[k][a].__setitem__(f,v))
        if key=='seek_file_calls':
            negative('outer wrong record '+str(i),lambda s,a=i:s['seek_file_calls'][a].__setitem__('record',0))
            for field,value in [('address',0),('bytes',8),('owner_local_address',0),('owner_local_bytes',8),('before','01000000'),('owner_kind','unknown')]:
                negative('outer%d output %s'%(i,field),lambda s,a=i,f=field,v=value:s['seek_file_calls'][a]['output'].__setitem__(f,v))
        else:
            negative('inner changed initial error '+str(i),lambda s,a=i:s['mapped_seek_calls'][a].__setitem__('error_before','01000000'))
negative('missing true length return',lambda s:s['file_length_returns'].clear())
for field,value in [('normal_return',False),('return_stack',1),('result',2261),('record_after','00'*40)]:
    negative('length returned '+field,lambda s,f=field,v=value:s['file_length_returns'][0].__setitem__(f,v))
for field,value in [('formals',[15]),('hidden_inputs',['ecx']),('stack_pointer',1)]:
    negative('reader true entry '+field,lambda s,f=field,v=value:s['big_endian_entry'].__setitem__(f,v))
def coherent_swap(state):
    for name in('seek_file_calls','mapped_seek_calls'):state[name][1],state[name][2]=state[name][2],state[name][1]
negative('coherently swapped end and restore calls',coherent_swap)
def copied_local(state):
    for i in(1,2):
        outer=state['seek_file_calls'][i];inner=state['mapped_seek_calls'][i]
        outer['formals'][3]-=4;inner['formals'][3]-=4
        outer['output']['address']-=4;outer['output']['owner_local_address']-=4
negative('coherent copied reused end-error local',copied_local)
def changed_final(state):
    for name in('seek_file_calls','mapped_seek_calls'):
        row=state[name][-1];data=bytearray.fromhex(row['record_after']);struct.pack_into('<I',data,28,1);row['record_after']=data.hex();row['result']=1
    data=bytearray.fromhex(state['file_records'][14]);struct.pack_into('<I',data,28,1);state['file_records'][14]=data.hex()
negative('coherent incorrect final reset state',changed_final)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/mapped_seek_state.py',root/'scripts/archive_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Repeated-seek boundary passes; %d focused negatives rejected.'%len(checks))
