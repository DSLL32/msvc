#!/usr/bin/env python3
"""Focused packing entry/first-call/state controls without executing targets."""
import argparse,copy,json,re,tempfile,time
from pathlib import Path
from provenance import sha,PROJECT
from packing_state import validate_endpoint
from importlib.util import spec_from_file_location,module_from_spec
spec=spec_from_file_location('normalizer_controls',Path(__file__).with_name('check-normalizer-negative.py'));mod=module_from_spec(spec);spec.loader.exec_module(mod);Overlay=mod.Overlay

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();c=json.loads((a.original/'candidate.json').read_text());started=time.perf_counter();validate_endpoint(a.original,a.native,c);controls=[]
 def reject(label,original,native,candidate):
  try:validate_endpoint(original,native,candidate)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(label)
 for label,name,change in [
  ('entry-omission','packing-entries.json',lambda x:x.clear()),
  ('entry-physical-PC','packing-entries.json',lambda x:x[0].__setitem__('pc',0)),
  ('entry-formals','packing-entries.json',lambda x:x[0]['formal_inventory'].append(dict(name='fake',type='int'))),
  ('entry-state','packing-entries.json',lambda x:x[0]['compiler_state']['option_state']['initialized'].__setitem__('packing_stack',1)),
  ('allocator-request','state.json',lambda x:x['args'].__setitem__('bytes',12)),
  ('allocator-formal-omission','state.json',lambda x:x['formal_inventory'].clear()),
  ('allocator-formal-type','state.json',lambda x:x['formal_inventory'][0].__setitem__('type','int')),
  ('allocator-physical-trap','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
  ('allocator-caller','state.json',lambda x:x['caller'].__setitem__('symbol','parse_command_line')),
  ('allocator-caller-formals','state.json',lambda x:x['caller'].__setitem__('formal_inventory',[dict(name='fake',type='int')])),
  ('target-write','state.json',lambda x:x['compiler_state']['option_state']['initialized'].__setitem__('packing_stack',1)),
  ('neighbor-write','state.json',lambda x:x['compiler_state']['initialized'].__setitem__('c9_il',0)),
  ('snapshot-write','state.json',lambda x:x['compiler_state']['option_state']['snapshot'].__setitem__('packing_stack',1)),
  ('FP-control','state.json',lambda x:x['command_line_fp'].__setitem__('cw',0x37f)),
 ]:
  data=json.loads((a.native/name).read_text());change(data)
  with tempfile.TemporaryDirectory(prefix='packing-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(data));reject(label,a.original,Overlay(a.native,{name:path}),c)
 seq=int(re.findall(r'^trap seq=(\d+) rva=000134c9 ',(a.original/'events.txt').read_text(),re.M)[0])
 for label,name,offset in [
  ('original-data-write','compiler-data-%03d.bin'%seq,0),
  ('original-BSS-write','compiler-bss-%03d.bin'%seq,0),
  ('original-snapshot-write','compiler-snapshot-%03d.bin'%seq,0),
  ('original-outside-write','state-%03d.bin'%seq,0),
  ('original-locale-write','locale-packing-allocator-ctype.bin',200),
  ('original-error-write','locale-packing-allocator-errors.bin',0),
  ('original-FP-write','locale-packing-allocator-fp.bin',4),
 ]:
  data=bytearray((a.original/name).read_bytes());data[offset]^=1
  with tempfile.TemporaryDirectory(prefix='packing-control-',dir=a.original.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(data);reject(label,Overlay(a.original,{name:path}),a.native,c)
 for label,change in [('wrong-candidate-extent',lambda x:x['functions'][-1]['chunks'][0].__setitem__(1,0x134c8)),('wrong-allocator-frontier',lambda x:x['expected_frontier'].__setitem__('rva',0x134ca)),('wrong-size-contract',lambda x:x['packing_observers'].__setitem__('request_bytes',12))]:
  changed=copy.deepcopy(c);change(changed);reject(label,a.original,a.native,changed)
 result=dict(status='all focused packing controls rejected',count=len(controls),controls=controls,seconds=time.perf_counter()-started,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(path.relative_to(PROJECT)):sha(path)for path in [Path(__file__),PROJECT/'scripts/packing_state.py']});a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))
if __name__=='__main__':main()
