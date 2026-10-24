#!/usr/bin/env python3
"""Reject damaged header returns and all physical object-reader inputs."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import time
from boundary_state import compare
from swap_result_state import compare as compare_result

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',required=True,type=Path)
parser.add_argument('--native',required=True,type=Path)
parser.add_argument('--report',required=True,type=Path)
args=parser.parse_args();frontier='_read_object_file@12'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in('byte_swap_returns','big_endian_returns','mac_resource_returns'):
    negative('missing '+key,lambda s,k=key:s[k].clear())
    for field,value in [('normal_return',False),('return_stack',1),('result',1),('stack_pointer',1),('hidden_inputs',['ecx']),('formals',[])]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k][0].__setitem__(f,v))
negative('Mac raw AL contradiction',lambda s:s['mac_resource_returns'][0].__setitem__('raw_result',1))
for i in range(3):
    negative('object physical formal%d'%i,lambda s,j=i:s['object_reader_entry']['formals'].__setitem__(j,0))
    negative('object frontier formal%d'%i,lambda s,j=i:s['frontier_inputs']['formals'].__setitem__(j,0))
for field,value in [('stack_pointer',1),('hidden_inputs',['ecx']),('formals',[])]:
    negative('object entry '+field,lambda s,f=field,v=value:s['object_reader_entry'].__setitem__(f,v))
negative('object frontier hidden input',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
for field,value in [('address',1),('bytes',4),('value',1)]:
    negative('object mode '+field,lambda s,f=field,v=value:s['object_read_mode'].__setitem__(f,v))
def coherent_wrong_argument_order(state):
    for value in(state['object_reader_entry'],state['frontier_inputs']):value['formals'][1],value['formals'][2]=value['formals'][2],value['formals'][1]
negative('coherent wrong object argument order',coherent_wrong_argument_order)
for key in('big_endian_read_entry','byte_swap_entry'):
    negative('lost historical '+key,lambda s,k=key:s[k].__setitem__('formals',[]))
negative('lost historical normal mapped read',lambda s:s['big_endian_read_returns'].clear())
negative('lost actual DWORD output',lambda s:s['big_endian_read_returns'][0]['output'].__setitem__('after','00000000'))

# Exercise actual-original metadata checks directly without changing any raw
# evidence file. These are typed-validator controls, not fabricated captures.
entry,=[e for e in events if e['event']=='big_endian_entry'];mac,=[e for e in events if e['event']=='mac_resource_entry']
output=(args.original/'big-endian-read-0-after.bin').read_bytes()
for event in('byte_swap_return','big_endian_return','mac_resource_return'):
    for field,value in [('return_stack',1),('result',1),('entry_stack',1)]:
        changed=copy.deepcopy(events);row,=[e for e in changed if e['event']==event];row[field]=value
        try:compare_result(args.original,native,trap,changed,args.native,entry,mac,output)
        except(AssertionError,KeyError,ValueError):checks.append('original '+event+' '+field)
        else:raise AssertionError('original mutation accepted: '+event+' '+field)
for i in range(3):
    changed=copy.deepcopy(trap);changed['object_read_formals']['formals'][i]=0
    try:compare_result(args.original,native,changed,events,args.native,entry,mac,output)
    except(AssertionError,KeyError,ValueError):checks.append('original object formal%d'%i)
    else:raise AssertionError('original object input accepted: '+str(i))

# Only the classifier's AL is returned. A different own upper EAX is allowed
# by the observed TEST AL; no DWORD helper result is narrowed this way.
state=copy.deepcopy(native);state['mac_resource_returns'][0]['raw_result']=0xa1b2c300
compare(frontier,args.original,args.native,trap,state)
state=copy.deepcopy(native)
for value in(state['big_endian_output'],state['big_endian_read_returns'][0]['output']):value['before']='a1b2c3d4'
compare(frontier,args.original,args.native,trap,state)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/swap_result_state.py',root/'scripts/big_endian_state.py',root/'scripts/mapped_seek_state.py',root/'scripts/archive_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,own_upper_eax_positive=True,own_uninitialized_output_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_scope_positives=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Swap returns and object inputs pass; %d affected controls rejected.'%len(checks))
