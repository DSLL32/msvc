#!/usr/bin/env python3
"""Reject wrong actual machine-detector inputs and owner/caller metadata."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import time
from boundary_state import compare
from detect_machine_state import compare as compare_detect

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',required=True,type=Path)
parser.add_argument('--native',required=True,type=Path)
parser.add_argument('--report',required=True,type=Path)
args=parser.parse_args();frontier='_detect_input_machine@8'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in('detect_machine_entry','input_machine_entry','frontier_inputs'):
    for i in range(2):negative(key+' formal%d'%i,lambda s,k=key,j=i:s[k]['formals'].__setitem__(j,0))
    negative('missing '+key+' formal',lambda s,k=key:s[k]['formals'].pop())
for key in('detect_machine_entry','input_machine_entry'):
    negative(key+' wrong entry stack',lambda s,k=key:s[k].__setitem__('stack_pointer',s[k]['stack_pointer']+4))
    negative(key+' invented hidden receiver',lambda s,k=key:s[k].__setitem__('hidden_inputs',['ecx']))
negative('frontier hidden receiver',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
def swapped(state):
    for row in(state['detect_machine_entry'],state['frontier_inputs']):row['formals'].reverse()
negative('coherently reversed detector inputs',swapped)
def copied_context(state):
    address=state['object_reader_entry']['formals'][2]
    for row in(state['detect_machine_entry'],state['input_machine_entry'],state['frontier_inputs']):row['formals'][0]=address
negative('coherent detector context changed to library',copied_context)
negative('premature machine initialization',lambda s:s['object_read_mode'].__setitem__('value',1))
negative('lost actual preceding DWORD result',lambda s:s['byte_swap_returns'][0].__setitem__('result',0))
for field,value in [('esp',1),('detect_machine_formals',dict(formals=[0,0],hidden_inputs=[])),('frontier_inputs',dict(formals=[0,0],hidden_register_inputs=[]))]:
    changed=copy.deepcopy(trap);changed[field]=value
    try:compare_detect(args.original,native,changed,events,args.native)
    except(AssertionError,KeyError,ValueError):checks.append('original actual '+field)
    else:raise AssertionError('original detector input accepted: '+field)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/detect_machine_state.py',root/'scripts/input_machine_state.py',root/'scripts/swap_result_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Detector frontier passes; %d affected controls rejected.'%len(checks))
