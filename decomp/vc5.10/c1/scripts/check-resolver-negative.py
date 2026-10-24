#!/usr/bin/env python3
"""Focused actual local-span, formal, state and normal-only controls."""
import argparse,json,re,tempfile,time
from pathlib import Path
from integer_policy import PROJECT
from provenance import sha
from resolver_state import validate_copy

class Overlay:
 def __init__(self,base,files):self.base,self.files=base,files
 def __truediv__(self,name):return self.files.get(name,self.base/name)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 c=json.loads((a.original/'candidate.json').read_text());started=time.perf_counter();validate_copy(a.original,a.native,c);controls=[]
 def rejected(label,scope,op):
  try:op()
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(control=label,scope=scope))
 def native(label,name,change):
  value=json.loads((a.native/name).read_text());change(value)
  with tempfile.TemporaryDirectory(prefix='resolver-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(value));overlay=Overlay(a.native,{name:path})
   rejected(label,'native',lambda:validate_copy(a.original,overlay,c))
 for label,name,change in [
  ('duplicate-entry','resolver-entries.json',lambda x:x.append(x[0])),
  ('entry-formal-omission','resolver-entries.json',lambda x:x[0]['formal_inventory'].pop()),
  ('entry-size','resolver-entries.json',lambda x:x[0]['args'].__setitem__('size',519)),
  ('entry-input-NUL','resolver-entries.json',lambda x:x[0]['input_bytes'].__setitem__(-1,1)),
  ('entry-physical-PC','resolver-entries.json',lambda x:x[0].__setitem__('pc',0)),
  ('helper-formal-omission','state.json',lambda x:x['formal_inventory'].clear()),
  ('helper-invented-formal','state.json',lambda x:x['args'].__setitem__('fake',0)),
  ('helper-physical-trap','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
  ('caller-formal-omission','state.json',lambda x:x['caller']['formal_inventory'].pop()),
  ('caller-output-role','state.json',lambda x:x['caller']['args'].__setitem__('output',0)),
  ('local-span','state.json',lambda x:x['resolver_local_buffer'].__setitem__('bytes',259)),
  ('local-address-identity','state.json',lambda x:x['resolver_local_buffer'].__setitem__('address',0)),
  ('copied-NUL','state.json',lambda x:x['resolver_path_bytes'].__setitem__(-1,1)),
  ('modeled-neighbor','state.json',lambda x:x['compiler_state']['bss'].__setitem__('debug_break',1)),
  ('LastError','state.json',lambda x:x.__setitem__('win32_last_error',1)),
  ('locale-errors','state.json',lambda x:x['resolver_locale']['errors'].__setitem__(0,1)),
  ('FP-status','state.json',lambda x:x['command_line_fp'].__setitem__('sw',0)),
 ]:native(label,name,change)
 seq=int(re.findall(r'^trap seq=(\d+) rva=0001344d ',(a.original/'events.txt').read_text(),re.M)[0])
 for label,name,offset in [
  ('entry-record-ABI','resolver-entry-registers.bin',48),
  ('original-input-NUL','resolver-entry-input.bin',8),
  ('original-copy-NUL','resolver-normalize-input.bin',8),
  ('original-target-state','locale-resolver-normalize-target-data.bin',0),
  ('original-snapshot-neighbor','locale-resolver-normalize-target-snapshot.bin',0),
  ('original-errors','locale-resolver-normalize-errors.bin',0),
  ('original-FP','locale-resolver-normalize-fp.bin',4),
  ('original-CRT-outside-state','state-%03d.bin'%seq,0x90),
 ]:
  data=bytearray((a.original/name).read_bytes());data[offset]^=1
  with tempfile.TemporaryDirectory(prefix='resolver-control-',dir=a.original.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(data);overlay=Overlay(a.original,{name:path})
   rejected(label,'original',lambda:validate_copy(overlay,a.native,c))
 changed=dict(c);changed['allowed_probes']=['normal','restore'];rejected('opaque-restore-mode','candidate',lambda:validate_copy(a.original,a.native,changed))
 result=dict(status='all affected resolver controls rejected',count=len(controls),seconds=time.perf_counter()-started,controls=controls,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(path):sha(path)for path in [Path(__file__),PROJECT/'scripts/resolver_state.py']})
 a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))

if __name__=='__main__':main()
