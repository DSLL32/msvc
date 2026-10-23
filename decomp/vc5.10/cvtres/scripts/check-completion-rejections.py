#!/usr/bin/env python3
"""Canonical rich COFF/stream/status guards over completed actual raw pairs."""
import argparse,copy,json,shutil,tempfile
from pathlib import Path
from evidence import compare_frontier
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
config=json.loads((a.original/'configuration.json').read_text());compare_frontier(config,a.original,a.native)
results=[]
for label in ('both-wrong-COFF','both-wrong-stdout','both-wrong-status'):
 with tempfile.TemporaryDirectory(prefix='completion-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
  root=Path(temporary);old=root/'original';new=root/'native';old.mkdir();new.mkdir()
  for source,dest,names in [(a.original,old,['events.txt','stdout.bin','stderr.bin','rich.obj']),(a.native,new,['state.json','gdb.stdout','trace.txt','stdout.bin','stderr.bin','rich.obj'])]:
   for name in names:shutil.copyfile(source/name,dest/name)
  if label=='both-wrong-COFF':
   for directory in [old,new]:
    data=bytearray((directory/'rich.obj').read_bytes());data[-1]^=1;(directory/'rich.obj').write_bytes(data)
  elif label=='both-wrong-stdout':
   for directory in [old,new]:(directory/'stdout.bin').write_bytes(b'wrong\n')
  else:
   (old/'events.txt').write_text((old/'events.txt').read_text().replace('exit 0\n','exit 1\n'))
   state=json.loads((new/'state.json').read_text());state['exit_status']=1;(new/'state.json').write_text(json.dumps(state))
  try:compare_frontier(config,old,new)
  except AssertionError as error:results.append(dict(name=label,rejected=True,reason=str(error)))
  else:raise AssertionError(label+' accepted')
a.output.write_text(json.dumps(dict(scope='Actual completed raw pair copies; both wrong sides must fail untouched canonical reference',cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
