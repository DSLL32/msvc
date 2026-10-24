#!/usr/bin/env python3
"""Exercise the actual object-to-machine ABI, global and inherited state."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import time
from boundary_state import compare
from input_machine_state import compare as compare_machine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--original',required=True,type=Path)
parser.add_argument('--native',required=True,type=Path)
parser.add_argument('--report',required=True,type=Path)
args=parser.parse_args();frontier='_check_input_machine@8'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in('input_machine_entry','frontier_inputs'):
    for i in range(2):negative(key+' formal%d'%i,lambda s,k=key,j=i:s[k]['formals'].__setitem__(j,0))
    negative('missing '+key+' formal',lambda s,k=key:s[k]['formals'].pop())
negative('machine hidden receiver',lambda s:s['input_machine_entry'].__setitem__('hidden_inputs',['ecx']))
negative('frontier hidden receiver',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
negative('wrong caller frame',lambda s:s['input_machine_entry'].__setitem__('stack_pointer',s['input_machine_entry']['stack_pointer']+4))
for i in range(3):negative('historical object formal%d'%i,lambda s,j=i:s['object_reader_entry']['formals'].__setitem__(j,0))
negative('historical object hidden input',lambda s:s['object_reader_entry'].__setitem__('hidden_inputs',['ecx']))
negative('lost parent entry',lambda s:s['object_reader_entry'].__setitem__('stack_pointer',1))
for field,value in [('address',1),('bytes',4),('value',1)]:
    negative('machine init flag '+field,lambda s,f=field,v=value:s['object_read_mode'].__setitem__(f,v))
def coherent_swapped_inputs(state):
    for row in(state['input_machine_entry'],state['frontier_inputs']):row['formals'].reverse()
negative('coherent reversed context and argument',coherent_swapped_inputs)
def copied_context(state):
    address=state['object_reader_entry']['formals'][2]
    state['input_machine_entry']['formals'][0]=address;state['frontier_inputs']['formals'][0]=address
    state['object_reader_entry']['formals'][1]=address
negative('coherent context changed to library owner',copied_context)
def wrong_cursor(state):
    data=bytearray.fromhex(state['file_records'][14]);struct.pack_into('<I',data,28,8);state['file_records'][14]=data.hex()
negative('current cursor confused with archive history',wrong_cursor)
negative('lost mapped DWORD normal return',lambda s:s['big_endian_read_returns'].clear())
negative('wrong historical DWORD return',lambda s:s['byte_swap_returns'][0].__setitem__('result',0))
negative('wrong historical Mac AL',lambda s:s['mac_resource_returns'][0].__setitem__('raw_result',1))
for field,value in [('esp',1),('input_machine_formals',dict(formals=[0,0],hidden_inputs=[])),('frontier_inputs',dict(formals=[0,0],hidden_register_inputs=[]))]:
    changed=copy.deepcopy(trap);changed[field]=value
    try:compare_machine(args.original,native,changed,events,args.native)
    except(AssertionError,KeyError,ValueError):checks.append('original actual '+field)
    else:raise AssertionError('original input mutation accepted: '+field)
state=copy.deepcopy(native);state['mac_resource_returns'][0]['raw_result']=0xa1b2c300
compare(frontier,args.original,args.native,trap,state)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/input_machine_state.py',root/'scripts/swap_result_state.py',root/'scripts/big_endian_state.py',root/'scripts/mapped_seek_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,own_upper_eax_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_scope_positive=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Object/machine frontier passes; %d affected controls rejected.'%len(checks))
