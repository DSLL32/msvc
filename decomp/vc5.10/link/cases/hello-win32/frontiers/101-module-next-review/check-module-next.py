#!/usr/bin/env python3
"""Focused next-return, constructor allocation and new ownership controls."""
import argparse,copy,hashlib,json,struct,time
from pathlib import Path
from boundary_state import compare as compare_full
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
events=[json.loads(l)for l in(args.original/'events.jsonl').read_text().splitlines()];trap,=[e for e in events if e['event']=='trap'];state=json.loads((args.native/'state.json').read_text());start=time.perf_counter()
compare_full('_allocate_module_index',args.original,args.native,trap,state);positive=time.perf_counter()-start;negative_start=time.perf_counter();rejected=[]
def negative(label,mutate):
 t,s=copy.deepcopy(trap),copy.deepcopy(state);mutate(t,s)
 try:compare_full('_allocate_module_index',args.original,args.native,t,s)
 except(AssertionError,KeyError,ValueError):rejected.append(label);return
 raise AssertionError('mutation accepted: '+label)
def field(label,owner,key,value):negative(label,lambda t,s:s[owner].__setitem__(key,value))
for key,value in [('module',1),('module_local_address',1),('module_local_bytes',1),('section_count',8),('symbol_count',38),('library',1),('header_address',1),('created_address',1),('created','00'),('contributions',1),('contribution_bytes',64),('contribution_data','01'+'00'*447)]:field('constructor '+key,'module_creation_current',key,value)
for offset in (0,4,8,0x2c,0x38,0x64,0x68,0x98):
 def change(t,s,j=offset):
  current=s['module_creation_current'];raw=bytearray.fromhex(current['record']);value=struct.unpack_from('<I',raw,j)[0]^0x100;struct.pack_into('<I',raw,j,value);current['record']=raw.hex()
  page=bytearray.fromhex(s['output_module_arenas'][0]['data']);page[156:312]=raw;s['output_module_arenas'][0]['data']=page.hex()
  if j in(0,4):current['names'][j//4]['address']=value
  if j==0x68:current['contributions']=value
 negative('coherent module/page DWORD'+hex(offset),change)
for key,value in [('slot',1),('address',0),('raw','00'*16),('data','00'*4095+'01')]:
 negative('contribution arena '+key,lambda t,s,k=key,v=value:s['module_creation_current']['contribution_arena'].__setitem__(k,v))
for offset in (312,4095):
 def tail(t,s,j=offset):
  raw=bytearray.fromhex(s['output_module_arenas'][0]['data']);raw[j]=1;s['output_module_arenas'][0]['data']=raw.hex()
 negative('unused module page byte'+str(offset),tail)
for key,value in [('normal_return',False),('return_stack',1),('result',1),('raw_result',1),('after','00'*16),('formals',[1]),('hidden_inputs',['ecx'])]:
 negative('next realreturn '+key,lambda t,s,k=key,v=value:s['module_next_returns'][0].__setitem__(k,v))
for key,value in [('normal_return',False),('result',1),('return_stack',1)]:
 negative('lookup realreturn '+key,lambda t,s,k=key,v=value:s['module_find_returns'][0].__setitem__(k,v))
for key,value in [('formals',[1]),('hidden_inputs',['ecx']),('stack_pointer',1)]:field('allocator actualABI '+key,'module_index_entry',key,value)
negative('allocator hidden original input',lambda t,s:t['frontier_inputs']['hidden_register_inputs'].append(dict(register='eax',value=1)))
negative('allocator hidden native input',lambda t,s:s['frontier_inputs']['hidden_register_inputs'].append(dict(register='eax',value=1)))
negative('original allocator callerreturn',lambda t,s:t['stack'].__setitem__(0,1))
negative('native allocator callerreturn',lambda t,s:s['stack'].__setitem__(7,1))
negative('copied filename aliases borrowed input',lambda t,s:s['module_creation_current']['names'][0].__setitem__('address',s['input_module_entry']['formals'][0]))
negative('lost name allocation generation',lambda t,s:s['duplicate_calls'].pop())
negative('lost initialized next return',lambda t,s:s['module_next_returns'].clear())
# Own upper EAX bits do not enter the BYTE result consumed by actual AL tests.
scope=copy.deepcopy(state);scope['module_next_returns'][0]['raw_result']=0x12345600;compare_full('_allocate_module_index',args.original,args.native,trap,scope)
# Retired enum storage is never re-read as live at this later trap. Its real
# call/return snapshots remain checked above; no new output-only bytes compared.
root=Path(__file__).resolve().parents[1];files=['scripts/check-module-next.py','scripts/module_creation_state.py','scripts/module_enumerator_state.py','scripts/module_constructor_state.py','scripts/hello_boundary.py','scripts/section_state.py','scripts/context_heap.py','scripts/input_open_state.py','scripts/capture-help.c','scripts/native-frontier.py']
args.report.write_text(json.dumps(dict(positive=True,own_upper_EAX_scope_positive=True,fresh_negative_count=len(rejected),rejected=rejected,elapsed_seconds=dict(positive=positive,affected_negatives_and_scope=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in files}),indent=2)+'\n');print('Actual next/lookup returns, whole module/contribution/name generations +%d affected negatives PASS'%len(rejected))
