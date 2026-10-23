#!/usr/bin/env python3
"""Verify a captured typed boundary and reject allocation/callback mutations."""
import argparse
import copy
import json
from pathlib import Path
import struct
from boundary_state import compare

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native',required=True,type=Path)
parser.add_argument('--original',required=True,type=Path)
parser.add_argument('--frontier',required=True)
parser.add_argument('--report',required=True,type=Path)
args=parser.parse_args()
events=[json.loads(line) for line in (args.original/'events.jsonl').read_text().splitlines()]
original,=[event for event in events if event['event']=='trap']
native=json.loads((args.native/'state.json').read_text())
compare(args.frontier,args.original,args.native,original,native)
checks=[]
def negative(label,change):
    state=copy.deepcopy(native);change(state)
    try:compare(args.frontier,args.original,args.native,original,state)
    except (AssertionError,KeyError):checks.append(label);return
    raise AssertionError('mutation accepted: '+label)
for field,value in [('x87_control_word',0x37f),('crt_fmode',0x4000),('crt_commode',1),('crt_adjust_fdiv',1),('initerm_end_index',4)]:
    if field in native:negative(field,lambda s,k=field,v=value:s.__setitem__(k,v))
negative('missing initializer call',lambda s:s['initerm_calls'].pop())
negative('reversed initializer calls',lambda s:s['initerm_calls'].reverse())
negative('wrong argv',lambda s:s['crt_argv'].__setitem__(1,'/help'))
for index,call in enumerate(native.get('operator_new_calls',[])):
    negative('wrong allocation size '+str(index),lambda s,i=index:s['operator_new_calls'][i].__setitem__('bytes',1))
    negative('wrong allocation result '+str(index),lambda s,i=index:s['operator_new_calls'][i].__setitem__('result',0))
    symbol=['buffer12','buffer8','buffer4'][index]
    negative('wrong buffer capacity '+symbol,lambda s,k=symbol:s['app_globals'].__setitem__(k,s['app_globals'][k][:16]+'01000000'))
for index,call in enumerate(native.get('atexit_calls',[])):
    negative('wrong callback '+str(index),lambda s,i=index:s['atexit_calls'][i].__setitem__('callback','free_buffer_unknown'))
    negative('callback registration failure '+str(index),lambda s,i=index:s['atexit_calls'][i].__setitem__('result',-1))
if 'program_name' in native:
    negative('wrong program name',lambda s:s.__setitem__('program_name','link'))
    negative('lost local basename alias',lambda s:s.__setitem__('program_name_address',0))
if 'frontier_arguments' in native and 'a' in native['frontier_arguments']:
    negative('wrong file-cache limit',lambda s:s['frontier_arguments'].__setitem__('a',17))
if args.frontier in ('_run_linker@8','_initialize_repro','_collect_file_arguments@16','_parse_command_line@16','_parse_argument@8'):
    negative('wrong page size',lambda s:s.__setitem__('allocation_page_size',8192))
    negative('wrong physical memory',lambda s:s.__setitem__('maximum_physical_memory',0x4000000))
    negative('missing hardware call',lambda s:s['system_calls'].pop())
    negative('reordered hardware calls',lambda s:s['system_calls'].reverse())
    negative('wrong full hardware output',lambda s:s['system_calls'][0].__setitem__('after','00'*36))
    negative('wrong tool kind',lambda s:s.__setitem__('selected_tool_kind',2))
    negative('wrong tool name',lambda s:s.__setitem__('selected_tool_name','Helper'))
    negative('lost tool name table alias',lambda s:s.__setitem__('selected_tool_name_address',0))
    negative('wrong argc',lambda s:s['frontier_arguments'].__setitem__('argc',1))
    negative('lost argv alias',lambda s:s['frontier_arguments'].__setitem__('argv_address',0))
    negative('missing VC5 FILE buffer flag',lambda s:s.__setitem__('crt_stdout_bytes',(bytes.fromhex(s['crt_stdout_bytes'])[:12]+struct.pack('<I',10)+bytes.fromhex(s['crt_stdout_bytes'])[16:]).hex()))
    negative('broken FILE buffer alias',lambda s:s.__setitem__('crt_stdout_bytes',('00000000'+s['crt_stdout_bytes'][8:])))
    negative('broken file-record link',lambda s:s['file_records'].__setitem__(15,s['file_records'][15][:72]+'00000000'))
    negative('wrong file record count',lambda s:s['file_globals'].__setitem__('file_record_count',15))
if args.frontier in ('_collect_file_arguments@16','_parse_command_line@16','_parse_argument@8'):
    negative('unexpected repro directory',lambda s:s.__setitem__('repro_directory',1))
    negative('unexpected repro response',lambda s:s.__setitem__('repro_response',1))
    if 'environment_arguments' in native:negative('wrong parsing environments',lambda s:s['environment_arguments'].reverse())
if args.frontier=='_parse_command_line@16':
    negative('missing input prepass',lambda s:s.__setitem__('input_prepass',0))
    negative('unexpected prepass input',lambda s:s['input_files'].__setitem__(2,1))
if args.frontier=='_parse_argument@8':
    negative('wrong argument text',lambda s:s['argument_detail'].__setitem__('text','/help'))
    negative('wrong argument source',lambda s:s['argument_detail'].__setitem__('source',1))
    negative('lost argument argv alias',lambda s:s['argument_detail'].__setitem__('address',0))
    negative('premature argument tail',lambda s:s.__setitem__('argument_tail',1))
    negative('lost prepass',lambda s:s.__setitem__('input_prepass',0))
args.report.write_text(json.dumps(dict(frontier=args.frontier,positive=True,negative_count=len(checks),rejected=checks),indent=2)+'\n')
print('Typed boundary passes; %d semantic negatives rejected.'%len(checks))
