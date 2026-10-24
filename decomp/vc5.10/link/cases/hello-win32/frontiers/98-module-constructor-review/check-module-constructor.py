#!/usr/bin/env python3
"""Focused thiscall frontier/state controls; inherited prefix checked once."""
import argparse,copy,hashlib,json,time
from pathlib import Path
from boundary_state import compare as compare_full
from module_constructor_state import compare
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
events=[json.loads(l)for l in(args.original/'events.jsonl').read_text().splitlines()];trap,=[e for e in events if e['event']=='trap'];state=json.loads((args.native/'state.json').read_text());formals=trap['input_module_formals']['formals'];nformals=state['input_module_entry']['formals'];header=(args.original/'module-find-header.bin').read_bytes();start=time.perf_counter();compare_full('OutputLibrary::FindModule',args.original,args.native,trap,state);positive=time.perf_counter()-start;negative_start=time.perf_counter();rejected=[]
def check(t,s):compare(args.original,s,t,events,args.native,formals,nformals,header)
def negative(label,mutate):
 t,s=copy.deepcopy(trap),copy.deepcopy(state);mutate(t,s)
 try:check(t,s)
 except(AssertionError,KeyError,ValueError):rejected.append(label);return
 raise AssertionError('mutation accepted: '+label)
for field,value in [('stack_pointer',1),('this',1),('formals',[]),('other_hidden_inputs',['eax'])]:negative('native actualmethod '+field,lambda t,s,k=field,v=value:s['module_find_entry'].__setitem__(k,v))
for i in range(2):negative('nativephysicalformal%d'%i,lambda t,s,j=i:s['module_find_entry']['formals'].__setitem__(j,0x12345678))
for field,value in [('this',1),('formals',[]),('other_hidden_inputs',['eax'])]:negative('original actualmethod '+field,lambda t,s,k=field,v=value:t['module_find_formals'].__setitem__(k,v))
negative('original rawECX mismatches receiver',lambda t,s:t.__setitem__('ecx',1))
negative('original wrong methodreturn',lambda t,s:t['stack'].__setitem__(0,0x40ea28))
negative('original wrong stack',lambda t,s:t.__setitem__('esp',1))
for key in ['formals','hidden_register_inputs']:
 negative('native missingfrontier '+key,lambda t,s,k=key:s['frontier_inputs'].__setitem__(k,[]))
 negative('original missingfrontier '+key,lambda t,s,k=key:t['frontier_inputs'].__setitem__(k,[]))
negative('native unknown registerinput',lambda t,s:s['frontier_inputs']['hidden_register_inputs'].append(dict(register='edx',value=1)))
negative('native wrong formalidentity',lambda t,s:s['frontier_inputs']['formals'].__setitem__(1,1))
for field,value in [('header','00'*20),('created','00'),('mac_target',1),('native_ppc_symbol_maps',1)]:negative('current methodstate '+field,lambda t,s,k=field,v=value:s['module_find_state'].__setitem__(k,v))
# Incoming createdBYTE is still uninitialized output-only. Keep it consistent
# in the native live view while varying its own arbitrary value.
scope=copy.deepcopy(state);scope['input_module_created_output']['before']='5a';scope['module_find_state']['created']='5a';compare_full('OutputLibrary::FindModule',args.original,args.native,trap,scope)
root=Path(__file__).resolve().parents[1];files=['scripts/check-module-constructor.py','scripts/module_constructor_state.py','scripts/coff_reader_state.py','cases/hello-win32/create-module-input-contract.json','cases/hello-win32/find-module-input-contract.json'];args.report.write_text(json.dumps(dict(positive=True,own_uninitialized_created_scope_positive=True,fresh_negative_count=len(rejected),rejected=rejected,elapsed_seconds=dict(positive=positive,affected_negatives_and_scope=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in files}),indent=2)+'\n');print('Full constructor/thiscall positive +%d focused negatives pass.'%len(rejected))
