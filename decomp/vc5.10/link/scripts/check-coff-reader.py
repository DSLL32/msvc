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
args=parser.parse_args();frontier='_create_input_module@48'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in ('coff_read_entry','input_module_entry'):
    for field,value in [('stack_pointer',1),('formals',[]),('hidden_inputs',['ecx'])]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k].__setitem__(f,v))
for i in range(12):negative('module physicalformal%d'%i,lambda s,j=i:s['input_module_entry']['formals'].__setitem__(j,0x12345678))
for key in ('coff_read_output','input_module_created_output'):
    for field,value in [('address',1),('bytes',4),('owner_local_address',1),('owner_local_bytes',4),('before','')]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k].__setitem__(f,v))
for key in ('coff_read_returns','coff_header_returns'):
    negative('missing '+key,lambda s,k=key:s[k].clear())
    for field,value in [('normal_return',False),('return_stack',1),('stack_pointer',1),('formals',[]),('hidden_inputs',['ecx']),('result',1)]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k][0].__setitem__(f,v))
for field,value in [('after','00'*20),('record_after','00'*40),('owner_local_address',1)]:
    negative('COFF readoutput '+field,lambda s,f=field,v=value:s['coff_read_returns'][0]['output'].__setitem__(f,v))
negative('rawBYTE contradicts return',lambda s:s['coff_header_returns'][0].__setitem__('raw_result',0x12345601))
negative('lost complete readerheader result',lambda s:s['coff_header_returns'][0].__setitem__('header_after','00'*20))
negative('missing optional module input',lambda s:s['frontier_inputs'].__setitem__('formals',s['frontier_inputs']['formals'][:11]))
negative('invented module hidden receiver',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
def coherent_header(state):
    changed='4d'+state['coff_read_returns'][0]['output']['after'][2:]
    state['coff_read_returns'][0]['output']['after']=changed;state['coff_header_returns'][0]['header_after']=changed
negative('coherent wrong completeCOFFheader',coherent_header)
from coff_reader_state import check_ownership
config=json.loads((args.native/'functions.json').read_text());changed=copy.deepcopy(config)
next(r for r in changed['driver']if r['name']=='_read_coff_header@16')['end']=0x4611e0
try:check_ownership(changed)
except AssertionError:checks.append('extra readeralignment padding')
else:raise AssertionError('ownership mutation accepted')
# Neither raw upperEAX nor own incoming uninitialized outputbytes are inputs.
state=copy.deepcopy(native);state['coff_header_returns'][0]['raw_result']=0xa1b2c300
for key in ('coff_read_output','coff_header_output'):state[key]['before']='a1'*20
state['coff_read_returns'][0]['output']['before']='a1'*20
state['input_module_created_output']['before']='5a'
compare(frontier,args.original,args.native,trap,state)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/coff_reader_state.py',root/'scripts/x86_backend_state.py',root/'scripts/archive_state.py',root/'scripts/input_open_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,own_upper_eax_header_and_created_before_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_scope_positive=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('COFF reader/module boundary passes; %d affected negatives rejected.'%len(checks))
