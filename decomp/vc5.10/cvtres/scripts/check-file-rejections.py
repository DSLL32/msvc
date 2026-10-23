#!/usr/bin/env python3
"""Native-only semantic rejection checks using actual seek entry observations."""
import argparse
import copy
import json
from pathlib import Path
import re
from evidence import compare_parameters
from mask import load
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
c=load()[0];f=next(f for f in c['functions'] if f['name']=='seek_file');text=(a.original/'events.txt').read_text();capsule=re.search(r'^entry '+format(f['start'],'08x')+r'\n(.*?)(?=^entry [0-9a-f]+$|^trap |\Z)',text,re.M|re.S)[1];state=json.loads((a.native/'state.json').read_text());entry=next(e for e in state['entries'] if e['function']=='seek_file')
compare_parameters(c,f,capsule,entry,a.original,a.native,prefix='entry-')
tests=[('wrong-scalar-offset',lambda e:e['parameters']['1'].update(value=1),'scalar argument differs'),('wrong-file-flags',lambda e:e['parameters']['0'].update(os_flags=0x81),'descriptor flags differ'),('wrong-physical-position',lambda e:e['parameters']['0'].update(position=1),'physical position differs'),('wrong-file-path',lambda e:e['parameters']['0'].update(path='/tmp/unrelated/rich.res'),'not exact case file'),('wrong-buffer-bytes',lambda e:e['parameters']['0'].update(buffer_hex='00'),'buffer contents differ')]
results=[]
for name,edit,reason in tests:
 changed=copy.deepcopy(entry);edit(changed)
 try:compare_parameters(c,f,capsule,changed,a.original,a.native,prefix='entry-')
 except AssertionError as error:
  assert reason in str(error),(name,str(error));results.append(dict(name=name,rejected=True,reason=str(error)))
 else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(schema=1,scope='Typed semantic guards over actual immutable candidate entry, not synthetic execution',original=str(a.original.resolve()),native=str(a.native.resolve()),cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
