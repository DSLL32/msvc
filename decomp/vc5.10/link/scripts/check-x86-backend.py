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
args=parser.parse_args();frontier='_read_coff_header@16'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in ('x86_backend_returns','initialize_machine_returns'):
    negative(key+' missing',lambda s,k=key:s[k].clear())
    for field,value in [('normal_return',False),('return_stack',1),('stack_pointer',1),('formals',[0]),('hidden_inputs',['ecx']),('context_after','00'*664)]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k][0].__setitem__(f,v))
for field,value in [('address',1),('owner_local_address',1),('bytes',4),('owner_local_bytes',4),('before','01'),('after','00')]:
    negative('backend live output '+field,lambda s,f=field,v=value:s['x86_backend_returns'][0]['output'].__setitem__(f,v))
for field in ('process_relocations','classify_relocation'):
    negative('coherent wrong installed '+field,lambda s,f=field:(s['x86_backend_returns'][0].__setitem__(f,0),s['current_machine_state'].__setitem__(f,0)))
    negative('nonzero prior callback '+field,lambda s,f=field:s['relocation_callbacks_before'].__setitem__(f,1))
negative('lost initialized flag after genuine return',lambda s:s['initialize_machine_returns'][0].__setitem__('machine_initialized',0))
negative('lost current initialized flag',lambda s:s['current_machine_state'].__setitem__('machine_initialized',0))
negative('invented small alignment',lambda s:s['current_machine_state'].__setitem__('small_section_alignment',1))
for key in ('seek_file_calls','mapped_seek_calls'):
    negative('missing sixth '+key,lambda s,k=key:s[k].pop())
    for field,value in [('normal_return',False),('return_stack',1),('stack_pointer',1),('result',1),('hidden_inputs',['ecx']),('record_before','00'*40),('record_after','00'*40),('error_after','01000000')]:
        negative(key+' sixth '+field,lambda s,k=key,f=field,v=value:s[k][5].__setitem__(f,v))
    for i in range(4):negative(key+' sixth formal%d'%i,lambda s,k=key,j=i:s[k][5]['formals'].__setitem__(j,0x12345678))
for field,value in [('address',1),('owner_local_address',1),('owner_local_bytes',1),('before','01000000'),('owner_kind','expired.initializer')]:
    negative('sixth error ownership '+field,lambda s,f=field,v=value:s['seek_file_calls'][5]['output'].__setitem__(f,v))
for field,value in [('address',1),('bytes',4),('owner_local_address',1),('owner_local_bytes',24),('before','00'*19)]:
    negative('COFF output ownership '+field,lambda s,f=field,v=value:s['coff_header_output'].__setitem__(f,v))
for field,value in [('stack_pointer',1),('formals',[14,0,0,0]),('hidden_inputs',['ecx'])]:
    negative('COFF entry '+field,lambda s,f=field,v=value:s['coff_header_entry'].__setitem__(f,v))
negative('missing COFF alternate input',lambda s:s['frontier_inputs'].__setitem__('formals',s['frontier_inputs']['formals'][:3]))
negative('COFF hidden receiver',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
from x86_backend_state import check_ownership
config=json.loads((args.native/'functions.json').read_text())
for label,mutate in [('extra backend padding',lambda c:next(r for r in c['driver']if r['name']=='_initialize_x86_backend@8').__setitem__('end',0x431920)),('extra GPSIZE padding',lambda c:next(r for r in c['text_data']if r['start']==0x401430).__setitem__('end',0x401438))]:
    changed=copy.deepcopy(config);mutate(changed)
    try:check_ownership(changed)
    except(AssertionError,ValueError):checks.append(label)
    else:raise AssertionError('ownership mutation accepted: '+label)
state=copy.deepcopy(native)
state['coff_header_output']['before']='a1'*20
for key in ('x86_backend_returns','initialize_machine_returns'):state[key][0]['raw_result']=0x12345678
compare(frontier,args.original,args.native,trap,state)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/x86_backend_state.py',root/'scripts/delay_validator_state.py',root/'scripts/machine_init_state.py',root/'scripts/machine_result_state.py',root/'scripts/big_endian_state.py',root/'scripts/input_open_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,void_eax_and_own_uninitialized_header_positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives_and_scope_positive=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('X86 backend/lifetime transition passes; %d affected negatives rejected.'%len(checks))
