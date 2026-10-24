#!/usr/bin/env python3
"""Focused first-index writes, derived length call and module publication."""
import argparse,copy,hashlib,json,struct,time
from pathlib import Path
from boundary_state import compare as compare_full
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True);args=p.parse_args()
events=[json.loads(l)for l in(args.original/'events.jsonl').read_text().splitlines()];trap,=[e for e in events if e['event']=='trap'];state=json.loads((args.native/'state.json').read_text());start=time.perf_counter();compare_full('_allocate_module_section_headers@4',args.original,args.native,trap,state);positive=time.perf_counter()-start;negative_start=time.perf_counter();rejected=[]
def negative(label,mutate):
 t,s=copy.deepcopy(trap),copy.deepcopy(state);mutate(t,s)
 try:compare_full('_allocate_module_section_headers@4',args.original,args.native,t,s)
 except(AssertionError,KeyError,ValueError):rejected.append(label);return
 raise AssertionError('mutation accepted: '+label)
def nested(label,owner,key,value):negative(label,lambda t,s:s[owner].__setitem__(key,value))
for k,v in [('normal_return',False),('result',1),('return_stack',1),('formals',[1]),('hidden_inputs',['eax'])]:negative('index actualreturn '+k,lambda t,s,key=k,value=v:s['module_index_returns'][0].__setitem__(key,value))
for phase,key,value in [('before','reset',0),('after','reset',1),('before','value','01'+'00'*7),('after','value','01'+'00'*7),('before','descriptor','00'*12),('after','descriptor','00'*12)]:
 def field(t,s,p=phase,k=key,v=value):
  s['module_index_returns'][0][p][k]=v
  if p=='before':s['module_index_before'][k]=v
 negative('index '+phase+' '+key,field)
for offset in(0,7,8,200,3999):
 def byte(t,s,j=offset):
  a=bytearray.fromhex(s['module_index_returns'][0]['after']['array']);a[j]^=1;s['module_index_returns'][0]['after']['array']=a.hex()
 negative('index written/tail byte'+str(offset),byte)
for offset in(8,12,16,20,24,0x34,0x38,0x3c,0x40,0x60,0x64,0x68,0x98):
 def record(t,s,j=offset):
  c=s['module_creation_current'];raw=bytearray.fromhex(c['record']);struct.pack_into('<I',raw,j,struct.unpack_from('<I',raw,j)[0]^1);c['record']=raw.hex();page=bytearray.fromhex(s['output_module_arenas'][0]['data']);page[156:312]=raw;s['output_module_arenas'][0]['data']=page.hex()
 negative('coherent finalmodule/page '+hex(offset),record)
for k,v in [('module',1),('module_local_address',1),('module_local_bytes',8),('owner_function','create_input_module'),('header_address',1),('created_address',1),('created','00'),('library',1)]:nested('current readobject owner '+k,'module_creation_current',k,v)
for k,v in [('normal_return',False),('return_stack',1),('result',1),('created','00'),('record','00'*156),('header','00'*20),('library','00'*68)]:negative('constructor actualpublication '+k,lambda t,s,key=k,value=v:s['input_module_returns'][0].__setitem__(key,value))
for k,v in [('normal_return',False),('return_stack',1),('result',1),('formals',[15]),('hidden_inputs',['eax'])]:negative('second getlength '+k,lambda t,s,key=k,value=v:s['file_length_returns'][1].__setitem__(key,value))
for j in(6,7,8):
 negative('repeat seek missingnormalreturn'+str(j),lambda t,s,i=j:s['seek_file_calls'][i].__setitem__('normal_return',False))
 negative('repeat seek foreignerrorowner'+str(j),lambda t,s,i=j:s['seek_file_calls'][i]['output'].__setitem__('owner_local_address',1))
 negative('repeat mappedseek wrongresult'+str(j),lambda t,s,i=j:s['mapped_seek_calls'][i].__setitem__('result',1))
for k,v in [('formals',[1]),('hidden_inputs',['eax']),('stack_pointer',1)]:nested('pending header actualABI '+k,'module_headers_entry',k,v)
negative('original pendingheader pointer',lambda t,s:t['module_headers_formals'].__setitem__('formals',[1]))
negative('original pendingheader return',lambda t,s:t['stack'].__setitem__(0,1))
negative('native pendingheader return',lambda t,s:s['stack'].__setitem__(3,1))
negative('pendingheader hidden original',lambda t,s:t['frontier_inputs']['hidden_register_inputs'].append(dict(register='eax',value=1)))
negative('pendingheader hidden native',lambda t,s:s['frontier_inputs']['hidden_register_inputs'].append(dict(register='eax',value=1)))
# Values outside the first written8B are own malloc bytes, kept unchanged only
# within one execution. Changing both own snapshots must remain acceptable.
scope=copy.deepcopy(state)
for record in(scope['module_index_before'],scope['module_index_returns'][0]['before'],scope['module_index_returns'][0]['after']):
 a=bytearray.fromhex(record['array']);a[200]^=0x80;record['array']=a.hex()
compare_full('_allocate_module_section_headers@4',args.original,args.native,trap,scope)
root=Path(__file__).resolve().parents[1];files=['scripts/check-module-index.py','scripts/module_index_state.py','scripts/module_creation_state.py','scripts/module_enumerator_state.py','scripts/module_constructor_state.py','scripts/mapped_seek_state.py','scripts/hello_boundary.py','scripts/section_state.py','scripts/context_heap.py','scripts/library_creation_state.py','scripts/capture-help.c','scripts/native-frontier.py','cases/hello-win32/module-index-contract.json']
args.report.write_text(json.dumps(dict(positive=True,own_unused_array_scope_positive=True,fresh_negative_count=len(rejected),rejected=rejected,elapsed_seconds=dict(positive=positive,affected_negatives_and_scope=time.perf_counter()-negative_start,total=time.perf_counter()-start),premises={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in files}),indent=2)+'\n');print('First index/buffer/constructor publication/derived length +%d affected negatives PASS'%len(rejected))
