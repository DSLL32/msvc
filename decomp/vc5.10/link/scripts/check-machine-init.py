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
args=parser.parse_args();frontier='_validate_delay_load_options@4'
events=[json.loads(line)for line in(args.original/'events.jsonl').read_text().splitlines()]
trap,=[e for e in events if e['event']=='trap'];native=json.loads((args.native/'state.json').read_text());checks=[]
start=time.perf_counter();compare(frontier,args.original,args.native,trap,native)
positive=time.perf_counter()-start;negative_start=time.perf_counter()
def negative(label,mutate):
    state=copy.deepcopy(native);mutate(state)
    try:compare(frontier,args.original,args.native,trap,state)
    except(AssertionError,KeyError,ValueError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for key in ('initialize_machine_entry','validate_delay_entry'):
    for field,value in [('stack_pointer',1),('formals',[0]),('hidden_inputs',['ecx'])]:
        negative(key+' '+field,lambda s,k=key,f=field,v=value:s[k].__setitem__(f,v))
for field,value in [('address',1),('bytes',4),('value',1)]:
    negative('live incremental support BYTE '+field,lambda s,f=field,v=value:s['incremental_support_output'].__setitem__(f,v))
for field,value in [('delay_libraries','01000000'+'00'*8),('small_section_alignment',1),('entry_symbol',1),('entry_point_name',1)]:
    negative('initializer global '+field,lambda s,f=field,v=value:s['machine_init_globals'].__setitem__(f,v))
negative('missing whole initializer context',lambda s:s.__setitem__('initialize_machine_context_before','00'*664))
negative('premature machine initialized flag',lambda s:s['object_read_mode'].__setitem__('value',1))
negative('validator missing input',lambda s:s['frontier_inputs'].__setitem__('formals',[]))
negative('validator wrong context input',lambda s:s['frontier_inputs'].__setitem__('formals',[0]))
negative('validator invented hidden receiver',lambda s:s['frontier_inputs'].__setitem__('hidden_register_inputs',['ecx']))
# Exact ownership rejects code/literal alignment bytes independently of hashes.
from machine_init_state import check_ownership
config=json.loads((args.native/'functions.json').read_text())
for label,mutate in [('extra function padding',lambda c:next(r for r in c['driver']if r['name']=='_initialize_machine@4').__setitem__('end',0x4497b0)),('extra literal padding',lambda c:next(r for r in c['text_data']if r['start']==0x4019d4).__setitem__('end',0x4019dc)),('truncated literal',lambda c:next(r for r in c['text_data']if r['start']==0x4019d4).__setitem__('end',0x4019d8))]:
    changed=copy.deepcopy(config);mutate(changed)
    try:check_ownership(changed)
    except AssertionError:checks.append(label)
    else:raise AssertionError('ownership mutation accepted: '+label)
root=Path(__file__).resolve().parents[1]
paths=[Path(__file__),root/'scripts/machine_init_state.py',root/'scripts/machine_result_state.py',root/'scripts/boundary_state.py']
args.report.write_text(json.dumps(dict(positive=True,fresh_negative_count=len(checks),rejected=checks,elapsed_seconds=dict(positive=positive,negatives=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()for p in paths}),indent=2)+'\n')
print('Machine initializer boundary passes; %d affected negatives rejected.'%len(checks))
