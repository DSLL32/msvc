#!/usr/bin/env python3
"""Focused packing member, frame ownership/link and caller-store denials."""
import argparse,copy,json,tempfile,time
from pathlib import Path
from provenance import sha,PROJECT
from packing_frame_state import validate_endpoint
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
  ('member-entry-count','packing-member-entries.json',lambda x:x.clear()),
  ('member-module-token','packing-member-entries.json',lambda x:x[0].__setitem__('module_token',0)),
  ('member-receiver','packing-member-entries.json',lambda x:x[0]['args'].__setitem__('this',0)),
  ('member-this-formal','packing-member-entries.json',lambda x:x[0]['formal_inventory'].clear()),
  ('frame-owned-address','packing-member-entries.json',lambda x:x[0].__setitem__('new_frame_address',0)),
  ('frame-size','packing-member-entries.json',lambda x:x[0]['frame_layout'].__setitem__('bytes',16)),
  ('frame-alignment-offset','packing-member-entries.json',lambda x:x[0]['frame_layout'].__setitem__('alignment',8)),
  ('object-active-default','packing-member-entries.json',lambda x:x[0]['object'].__setitem__(0,1)),
  ('frame-entry-default','packing-member-entries.json',lambda x:x[0]['frame'].__setitem__(0,1)),
  ('member-return-count','packing-frame-returns.json',lambda x:x.clear()),
  ('member-return-EAX','packing-frame-returns.json',lambda x:x[0].__setitem__('eax',0)),
  ('member-return-cleanup','packing-frame-returns.json',lambda x:x[0].__setitem__('esp',0)),
  ('member-callee-save','packing-frame-returns.json',lambda x:x[0]['preserved'].__setitem__('edi',x[0]['preserved']['edi']^1)),
  ('member-arena-advance','packing-frame-returns.json',lambda x:x[0]['compiler_state']['bss']['arenas'][0].__setitem__('cursor',0)),
  ('active-link','packing-frame-returns.json',lambda x:x[0]['object'].__setitem__(0,x[0]['object'][0]^1)),
  ('frame-predecessor','packing-frame-returns.json',lambda x:x[0]['frame'].__setitem__(0,1)),
  ('untouched-free-head','packing-frame-returns.json',lambda x:x[0]['object'].__setitem__(4,1)),
  ('caller-alignment','packing-frame-ready.json',lambda x:x['frame'].__setitem__(4,4)),
  ('caller-tag','packing-frame-ready.json',lambda x:x['frame'].__setitem__(8,1)),
  ('neighbor-state','packing-frame-ready.json',lambda x:x['compiler_state']['bss']['arenas'][0].__setitem__('committed_limit',0)),
  ('frame-errors','packing-frame-ready.json',lambda x:x['errors'].__setitem__(0,1)),
  ('frame-FP','packing-frame-ready.json',lambda x:x['fp'].__setitem__('cw',0x37f)),
  ('frontier-alignment-argument','state.json',lambda x:x['args'].__setitem__('alignment',1)),
  ('frontier-tag-argument','state.json',lambda x:x['args'].__setitem__('tag',1)),
  ('frontier-formals','state.json',lambda x:x['formal_inventory'].pop()),
  ('frontier-physical-trap','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
  ('frontier-snapshot','state.json',lambda x:x['compiler_state']['option_state']['snapshot'].__setitem__('packing_stack',1)),
 ]:
  data=json.loads((a.native/name).read_text());change(data)
  with tempfile.TemporaryDirectory(prefix='packing-frame-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(data));reject(label,a.original,Overlay(a.native,{name:path}),c)
 for label,name,offset in [
  ('original-receiver','packing-frame-entry-registers.bin',16),
  ('original-RET','packing-frame-return-registers.bin',8),
  ('original-EAX','packing-frame-return-registers.bin',12),
  ('original-EDI','packing-frame-return-registers.bin',32),
  ('original-arena-advance','packing-frame-return-arena.bin',0),
  ('original-active-link','packing-frame-return-object.bin',0),
  ('original-free-head','packing-frame-return-object.bin',4),
  ('original-predecessor','packing-frame-return-memory.bin',0),
  ('original-caller-alignment','packing-frame-ready-memory.bin',4),
  ('original-caller-tag','packing-frame-ready-memory.bin',8),
  ('original-BSS-neighbor','locale-packing-frame-return-target-BSS.bin',0),
  ('original-snapshot','locale-packing-frame-ready-target-snapshot.bin',0),
  ('original-errors','locale-packing-frame-ready-errors.bin',0),
  ('original-FP','locale-packing-frame-return-fp.bin',4),
 ]:
  data=bytearray((a.original/name).read_bytes());data[offset]^=1
  with tempfile.TemporaryDirectory(prefix='packing-frame-control-',dir=a.original.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(data);reject(label,Overlay(a.original,{name:path}),a.native,c)
 for label,change in [('wrong-member-extent',lambda x:x['functions'][-1]['chunks'][0].__setitem__(1,0xd49a)),('wrong-frame-contract',lambda x:x['packing_frame_observers'].__setitem__('frame_bytes',16))]:
  changed=copy.deepcopy(c);change(changed);reject(label,a.original,a.native,changed)
 result=dict(status='all affected packing frame/return/caller controls rejected',count=len(controls),controls=controls,seconds=time.perf_counter()-started,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(path.relative_to(PROJECT)):sha(path)for path in [Path(__file__),PROJECT/'scripts/packing_frame_state.py']});a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))
if __name__=='__main__':main()
