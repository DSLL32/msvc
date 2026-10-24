#!/usr/bin/env python3
"""Focused actual call/output ownership controls at module lookup's first child."""
import argparse,copy,hashlib,json,time
from pathlib import Path
from boundary_state import compare as compare_full
from module_enumerator_state import compare
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args();events=[json.loads(l)for l in(args.original/'events.jsonl').read_text().splitlines()];trap,=[e for e in events if e['event']=='trap'];state=json.loads((args.native/'state.json').read_text());method,=[e for e in events if e['event']=='module_find_entry'];nmethod=state['module_find_entry'];start=time.perf_counter();compare_full('_initialize_module_enumerator@8',args.original,args.native,trap,state);positive=time.perf_counter()-start;rejected=[];negative_start=time.perf_counter()
def check(t,s):compare(args.original,s,t,events,args.native,method['stack_pointer'],nmethod)
def negative(label,mutate):
 t,s=copy.deepcopy(trap),copy.deepcopy(state);mutate(t,s)
 try:check(t,s)
 except(AssertionError,KeyError,ValueError):rejected.append(label);return
 raise AssertionError('mutation accepted: '+label)
for field,value in [('stack_pointer',1),('formals',[]),('hidden_inputs',['ecx'])]:negative('native actualenum '+field,lambda t,s,k=field,v=value:s['module_enum_entry'].__setitem__(k,v))
for i in range(2):negative('nativephysicalformal%d'%i,lambda t,s,j=i:s['module_enum_entry']['formals'].__setitem__(j,0x12345678))
for field,value in [('formals',[]),('hidden_inputs',['ecx'])]:negative('original actualenum '+field,lambda t,s,k=field,v=value:t['module_enum_formals'].__setitem__(k,v))
negative('original wrong return',lambda t,s:t['stack'].__setitem__(0,0x40f7b3))
negative('original wrongstack',lambda t,s:t.__setitem__('esp',1))
for key in ['formals','hidden_register_inputs']:
 negative('native wrongfrontier '+key,lambda t,s,k=key:s['frontier_inputs'].__setitem__(k,[1]))
 negative('original wrongfrontier '+key,lambda t,s,k=key:t['frontier_inputs'].__setitem__(k,[1]))
for field,value in [('address',1),('bytes',4),('owner_local_address',1),('owner_local_bytes',4),('before','00'*15),('initial_value_contract','inputbytes')]:negative('whole16B enumowner '+field,lambda t,s,k=field,v=value:s['module_enum_output'].__setitem__(k,v))
# Entire uninitialized output incoming content is each side's own data.
scope=copy.deepcopy(state);scope['module_enum_output']['before']='a5'*16;compare_full('_initialize_module_enumerator@8',args.original,args.native,trap,scope)
root=Path(__file__).resolve().parents[1];files=['scripts/check-module-enumerator.py','scripts/module_enumerator_state.py','scripts/module_constructor_state.py','cases/hello-win32/module-enumerator-input-contract.json'];args.report.write_text(json.dumps(dict(positive=True,own_uninitialized16B_scope_positive=True,fresh_negative_count=len(rejected),rejected=rejected,elapsed_seconds=dict(positive=positive,affected_negatives_and_scope=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in files}),indent=2)+'\n');print('Whole lookup/enum positive +%d focused negatives pass.'%len(rejected))
