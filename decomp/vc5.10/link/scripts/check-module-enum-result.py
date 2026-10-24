#!/usr/bin/env python3
"""Whole initialized enumerator, actual RET8 and next-call ABI controls."""
import argparse,copy,hashlib,json,struct,time
from pathlib import Path
from boundary_state import compare as compare_full
from module_enumerator_state import compare_completed
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args();events=[json.loads(l)for l in(args.original/'events.jsonl').read_text().splitlines()];trap,=[e for e in events if e['event']=='trap'];state=json.loads((args.native/'state.json').read_text());method,=[e for e in events if e['event']=='module_find_entry'];entry=method['stack_pointer'];formals=trap['module_enum_formals']['formals'];nmethod=state['module_find_entry'];nentry=state['module_enum_entry'];output=state['module_enum_output'];start=time.perf_counter();compare_full('_next_module@4',args.original,args.native,trap,state);positive=time.perf_counter()-start;negative_start=time.perf_counter();rejected=[]
def negative(label,mutate):
 t,s,e=copy.deepcopy(trap),copy.deepcopy(state),copy.deepcopy(events);mutate(t,s,e)
 try:compare_completed(args.original,s,t,e,args.native,entry,formals,nmethod,nentry,output)
 except(AssertionError,KeyError,ValueError):rejected.append(label);return
 raise AssertionError('mutation accepted: '+label)
negative('missing normalreturn',lambda t,s,e:s['module_enum_returns'].clear())
for field,value in [('normal_return',False),('return_stack',1),('stack_pointer',1),('formals',[]),('hidden_inputs',['ecx'])]:negative('actual initializerreturn '+field,lambda t,s,e,k=field,v=value:s['module_enum_returns'][0].__setitem__(k,v))
for field,value in [('stack_pointer',1),('formals',[]),('hidden_inputs',['ecx'])]:negative('actual nextentry '+field,lambda t,s,e,k=field,v=value:s['module_next_entry'].__setitem__(k,v))
for field,value in [('after','00'*16),('before','a5'*16),('address',1),('owner_local_bytes',4)]:negative('whole returnedrecord '+field,lambda t,s,e,k=field,v=value:s['module_enum_returns'][0]['output'].__setitem__(k,v))
for i in range(4):
 def change(t,s,e,j=i):
  raw=bytearray.fromhex(s['module_enum_current']);struct.pack_into('<I',raw,j*4,0x12345678);s['module_enum_current']=raw.hex();s['module_enum_returns'][0]['output']['after']=raw.hex()
 negative('coherent initializedrecordDWORD%d'%i,change)
def callback(t,s,e):
 raw=bytearray.fromhex(s['module_enum_current']);struct.pack_into('<I',raw,0,0x12345678);s['module_enum_current']=raw.hex();s['module_enum_returns'][0]['output']['after']=raw.hex();s['module_enum_callbacks']['next']=0x12345678
negative('coherent foreigncallback+record',callback)
negative('currentrecord not16B',lambda t,s,e:s.__setitem__('module_enum_current','00'*15))
negative('original wrongnextreturn',lambda t,s,e:t['stack'].__setitem__(0,0x40f7bd))
negative('original wrongnextstack',lambda t,s,e:t.__setitem__('esp',1))
negative('original wrongoneformal',lambda t,s,e:t['module_next_formals'].__setitem__('formals',[1]))
negative('native unknownnextregister',lambda t,s,e:s['frontier_inputs']['hidden_register_inputs'].append(dict(register='ecx',value=1)))
negative('original unknownnextregister',lambda t,s,e:t['frontier_inputs']['hidden_register_inputs'].append(dict(register='ecx',value=1)))
negative('original missing genuineenumreturn',lambda t,s,e:next(x for x in e if x['event']=='module_enum_return').__setitem__('event','module_probe'))
negative('original wrongRET8cleanup',lambda t,s,e:next(x for x in e if x['event']=='module_enum_return').__setitem__('return_stack',1))
# Entire own uninitialized incoming16B can vary; initializedafter must stay exact.
scope=copy.deepcopy(state);scope['module_enum_output']['before']='a5'*16;scope['module_enum_returns'][0]['output']['before']='a5'*16;compare_full('_next_module@4',args.original,args.native,trap,scope)
root=Path(__file__).resolve().parents[1];files=['scripts/check-module-enum-result.py','scripts/module_enumerator_state.py','scripts/module_constructor_state.py','scripts/capture-help.c','cases/hello-win32/module-enumerator-input-contract.json'];args.report.write_text(json.dumps(dict(positive=True,own_uninitialized16B_scope_positive=True,fresh_negative_count=len(rejected),rejected=rejected,elapsed_seconds=dict(positive=positive,affected_negatives_and_scope=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in files}),indent=2)+'\n');print('Whole enumerator result/RET8/next ABI +%d focused negatives pass.'%len(rejected))
