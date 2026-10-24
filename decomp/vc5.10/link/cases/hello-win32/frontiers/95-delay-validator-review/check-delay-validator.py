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
args=parser.parse_args();frontier='_initialize_x86_backend@8'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
negative('missing validator normal return',lambda s:s['validate_delay_returns'].clear())
for field,value in [('normal_return',False),('return_stack',1),('stack_pointer',1),('formals',[0]),('hidden_inputs',['ecx']),('context_after','00'*664)]:
    negative('validator return '+field,lambda s,f=field,v=value:s['validate_delay_returns'][0].__setitem__(f,v))
for field,value in [('stack_pointer',1),('formals',[0,0]),('hidden_inputs',['ecx'])]:
    negative('backend entry '+field,lambda s,f=field,v=value:s['x86_backend_entry'].__setitem__(f,v))
for field,value in [('address',1),('owner_local_address',1),('bytes',4),('owner_local_bytes',4),('before','01')]:
    negative('backend live BYTE '+field,lambda s,f=field,v=value:s['x86_incremental_support_output'].__setitem__(f,v))
negative('backend missing BYTE output formal',lambda s:s['frontier_inputs'].__setitem__('formals',s['frontier_inputs']['formals'][:1]))
negative('backend aliased output to context',lambda s:s['frontier_inputs']['formals'].__setitem__(1,s['frontier_inputs']['formals'][0]))
negative('backend invented hidden receiver',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
from delay_validator_state import check_ownership
from binary import PE
root=Path(__file__).resolve().parents[1];pe=PE(root.parents[2]/'toolchains/5.10/bin/LINK.EXE');config=json.loads((args.native/'functions.json').read_text())
mutations=[('extra validator padding',lambda c:next(r for r in c['driver']if r['name']=='_validate_delay_load_options@4').__setitem__('end',0x41fab0))]
for address in [0x403414,0x40340c,0x40142c,0x402954,0x4033fc]:
    mutations.append(('literal padding '+hex(address),lambda c,a=address:next(r for r in c['text_data']if r['start']==a).__setitem__('end',a+len(pe.string(a))+2)))
for label,mutate in mutations:
    changed=copy.deepcopy(config);mutate(changed)
    try:check_ownership(changed,pe)
    except(AssertionError,ValueError):checks.append(label)
    else:raise AssertionError('ownership mutation accepted: '+label)
state=copy.deepcopy(native);state['validate_delay_returns'][0]['raw_result']=0x12345678
compare(frontier,args.original,args.native,trap,state)
paths=[Path(__file__),root/'scripts/delay_validator_state.py',root/'scripts/machine_init_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,void_raw_eax_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_scope_positive=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Delay validator normal return/live BYTE passes; %d affected negatives rejected.'%len(checks))
