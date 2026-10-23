#!/usr/bin/env python3
"""Reject corrupted typed resource observations without executing originals."""
import argparse
import json
from pathlib import Path
import shutil
import tempfile
from accept import verify
from mask import load,sha
p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--audit',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
config=load()[0];verify(config,'load_message',a.original,a.native,a.audit)
results=[]
def capacity(o,n):
 d=json.loads((n/'state.json').read_text());d['resource_calls'][0]['capacity']=512;(n/'state.json').write_text(json.dumps(d))
def output_pointer(o,n):
 d=json.loads((n/'state.json').read_text());d['resource_calls'][0]['output']+=4;(n/'state.json').write_text(json.dumps(d))
def tail(o,n):
 d=json.loads((n/'state.json').read_text());r=d['message_returns'][1]['globals']['message_buffer'];r['raw_hex']=r['raw_hex'][:-2]+'01';(n/'state.json').write_text(json.dumps(d))
def pending(o,n):
 d=json.loads((n/'state.json').read_text());r=d['message_returns'][1]['crt']['recon_iob[1]'];r['buffer_hex']='00'+r['buffer_hex'][2:];(n/'state.json').write_text(json.dumps(d))
def callback(o,n):
 d=json.loads((n/'state.json').read_text());d['resource_calls'][0]['expected_loader']+=4;(n/'state.json').write_text(json.dumps(d))
def both_wrong(o,n):
 for directory in (o,n):(directory/'stdout.bin').write_bytes(b'wrong but equal\r\n')
for name,edit in [('wrong-capacity',capacity),('wrong-output-pointer',output_pointer),('changed-buffer-after-NUL',tail),('changed-pending-CRT-output',pending),('wrong-dynamic-callback',callback),('both-completions-wrong-output',both_wrong)]:
 with tempfile.TemporaryDirectory(prefix='message-negative-',dir=Path.home()/'.ai-tmp/msvc2/vc5.10/cvtres') as temporary:
  base=Path(temporary);o=base/'original';n=base/'native';shutil.copytree(a.original,o);shutil.copytree(a.native,n);edit(o,n)
  for directory in (o,n):
   r=json.loads((directory/'receipt.json').read_text());r['raw_sha256']={f:sha((directory/f).read_bytes()) for f in r['raw_sha256']};(directory/'receipt.json').write_text(json.dumps(r))
  try:verify(config,'load_message',o,n,a.audit)
  except AssertionError as error:
   reasons={'wrong-capacity':'scalar ABI differs','wrong-output-pointer':'output pointer differs','changed-buffer-after-NUL':'message_buffer','changed-pending-CRT-output':'CRT pending buffer differs','wrong-dynamic-callback':'native callback is not','both-completions-wrong-output':'untouched canonical master'}
   assert reasons[name] in str(error),(name,str(error))
   results.append(dict(name=name,rejected=True,reason=str(error)))
  else:raise AssertionError(name+' accepted')
a.output.write_text(json.dumps(dict(schema=1,cases=results),indent=2)+'\n');print(json.dumps(results,indent=2))
