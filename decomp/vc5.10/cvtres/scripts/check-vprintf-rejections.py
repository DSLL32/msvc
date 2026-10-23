#!/usr/bin/env python3
"""Reject altered actual variadic-call observations without rerunning originals."""
import argparse,copy,json,shutil,tempfile
from pathlib import Path
from evidence import compare_vprintf_observations
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
c=json.loads((a.original/'configuration.json').read_text());compare_vprintf_observations(c,a.original,a.native);state=json.loads((a.native/'state.json').read_text())
def wrong_pending(s):s['vprintf_returns'][0]['crt']['recon_iob[1]']['buffer_hex']='00'
tests=[('wrong-va-list-slot',lambda s:s['vprintf_calls'][0].update(args_pointer=s['vprintf_calls'][0]['args_pointer']+4)),('wrong-format',lambda s:s['vprintf_calls'][0].update(format_hex='00')),('wrong-argument-alias',lambda s:s['vprintf_calls'][0]['arguments'][0].update(pointer=1)),('wrong-return',lambda s:s['vprintf_returns'][0].update(return_value=999)),('wrong-pending-stdout',wrong_pending)]
results=[]
for name,edit in tests:
 with tempfile.TemporaryDirectory(prefix='varargs-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
  out=Path(temporary);changed=copy.deepcopy(state);edit(changed);(out/'state.json').write_text(json.dumps(changed))
  try:compare_vprintf_observations(c,a.original,out)
  except AssertionError as error:results.append(dict(name=name,rejected=True,reason=str(error)))
  else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(scope='Actual original/native cdecl and CRT FILE observations, semantic guard controls',cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
