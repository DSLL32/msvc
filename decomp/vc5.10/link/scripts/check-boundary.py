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
    negative(field,lambda s,k=field,v=value:s.__setitem__(k,v))
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
args.report.write_text(json.dumps(dict(frontier=args.frontier,positive=True,negative_count=len(checks),rejected=checks),indent=2)+'\n')
print('Typed boundary passes; %d semantic negatives rejected.'%len(checks))
