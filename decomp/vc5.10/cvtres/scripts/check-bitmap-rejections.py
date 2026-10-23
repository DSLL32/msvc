#!/usr/bin/env python3
"""Typed bitmap row argument controls over actual matched entry captures."""
import argparse,copy,json,re
from pathlib import Path
from evidence import compare_parameters
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
c=json.loads((a.original/'configuration.json').read_text());f=next(f for f in c['functions'] if f['name']=='convert_rows');text=(a.original/'events.txt').read_text();capsule=re.search(r'^entry '+format(f['start'],'08x')+r'\n(.*?)(?=^entry [0-9a-f]+$|^trap |\Z)',text,re.M|re.S)[1];state=json.loads((a.native/'state.json').read_text());entry=next(e for e in state['entries'] if e['function']=='convert_rows')
compare_parameters(c,f,capsule,entry,a.original,a.native,prefix='entry-')
def corrupt(e,index):
 raw=bytearray.fromhex(e['parameters'][index]['bytes_hex']);raw[0]^=1;e['parameters'][index]['bytes_hex']=raw.hex()
tests=[('consumed-WORD-depth',lambda e:e['parameters']['1'].update(value=8)),('row-stride',lambda e:e['parameters']['3'].update(value=1)),('input-pixels',lambda e:corrupt(e,'5')),('initial-output-pixels',lambda e:corrupt(e,'6')),('cursor-pointer-null',lambda e:e['parameters']['5'].update(cursor=0)),('output-not-writable',lambda e:e['parameters']['6'].update(permissions='r--p'))]
results=[]
for name,edit in tests:
 changed=copy.deepcopy(entry);edit(changed)
 try:compare_parameters(c,f,capsule,changed,a.original,a.native,prefix='entry-')
 except AssertionError as error:results.append(dict(name=name,rejected=True,reason=str(error)))
 else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(scope='Actual immutable bitmap row entry; WORD consumption excludes only ABI stack-slot padding',cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
