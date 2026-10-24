#!/usr/bin/env python3
"""Focused owned arena, allocation return and receiver-state denials."""
import argparse,copy,json,re,tempfile,time
from pathlib import Path
from provenance import sha,PROJECT
from packing_allocator_state import validate_endpoint
from importlib.util import spec_from_file_location,module_from_spec
spec=spec_from_file_location('packing_old_controls',Path(__file__).with_name('check-normalizer-negative.py'));mod=module_from_spec(spec);spec.loader.exec_module(mod);Overlay=mod.Overlay

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();c=json.loads((a.original/'candidate.json').read_text());started=time.perf_counter();validate_endpoint(a.original,a.native,c);controls=[]
 def reject(label,original,native,candidate):
  try:validate_endpoint(original,native,candidate)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(label)
 for label,name,change in [
  ('entry-count','packing-allocation-entries.json',lambda x:x.clear()),
  ('allocation-size','packing-allocation-entries.json',lambda x:x[0]['args'].__setitem__('bytes',7)),
  ('allocation-formal','packing-allocation-entries.json',lambda x:x[0]['formal_inventory'].clear()),
  ('allocation-entry-PC','packing-allocation-entries.json',lambda x:x[0].__setitem__('pc',0)),
  ('allocation-owned-role','packing-allocation-entries.json',lambda x:x[0].__setitem__('memory_address',0)),
  ('allocation-return','packing-allocation-returns.json',lambda x:x[0].__setitem__('eax',0)),
  ('return-cleanup','packing-allocation-returns.json',lambda x:x[0].__setitem__('esp',0)),
  ('return-callee-save','packing-allocation-returns.json',lambda x:x[0]['preserved'].__setitem__('esi',x[0]['preserved']['esi']^1)),
  ('cursor-increment','packing-allocation-returns.json',lambda x:x[0]['compiler_state']['bss']['arenas'][0].__setitem__('cursor',0)),
  ('untouched-committed-limit','packing-allocation-returns.json',lambda x:x[0]['compiler_state']['bss']['arenas'][0].__setitem__('committed_limit',0)),
  ('early-global-publication','packing-allocation-returns.json',lambda x:x[0]['compiler_state']['option_state']['initialized'].__setitem__('packing_stack',1)),
  ('returned-owned-bytes','packing-allocation-returns.json',lambda x:x[0]['memory'].__setitem__(0,1)),
  ('physical-member-count','packing-member-entries.json',lambda x:x.clear()),
  ('member-physical-ECX','packing-member-entries.json',lambda x:x[0].__setitem__('ecx',0)),
  ('member-this-formal','packing-member-entries.json',lambda x:x[0]['formal_inventory'].clear()),
  ('member-stack-argument','packing-member-entries.json',lambda x:x[0]['words'].append(1)),
  ('member-object-size','packing-member-entries.json',lambda x:x[0]['object_layout'].__setitem__('bytes',12)),
  ('member-object-offset','packing-member-entries.json',lambda x:x[0]['object_layout'].__setitem__('free_frames',8)),
  ('published-object','packing-member-entries.json',lambda x:x[0]['compiler_state']['option_state']['initialized'].__setitem__('packing_stack',0)),
  ('member-untouched-bytes','packing-member-entries.json',lambda x:x[0]['memory'].__setitem__(4,1)),
  ('member-errors','packing-member-entries.json',lambda x:x[0]['errors'].__setitem__(0,1)),
  ('member-FP','packing-member-entries.json',lambda x:x[0]['fp'].__setitem__('cw',0x37f)),
  ('frontier-this-value','state.json',lambda x:x['args'].__setitem__('this',0)),
  ('frontier-formals','state.json',lambda x:x['formal_inventory'].clear()),
  ('frontier-physical-trap','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
  ('frontier-snapshot','state.json',lambda x:x['compiler_state']['option_state']['snapshot'].__setitem__('packing_stack',1)),
 ]:
  data=json.loads((a.native/name).read_text());change(data)
  with tempfile.TemporaryDirectory(prefix='packing-alloc-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(data));reject(label,a.original,Overlay(a.native,{name:path}),c)
 for label,name,offset in [
  ('original-return-EAX','packing-allocation-return-registers.bin',12),
  ('original-return-ESI','packing-allocation-return-registers.bin',28),
  ('original-receiver-ECX','packing-allocation-member-registers.bin',16),
  ('original-cursor','packing-allocation-return-arena.bin',0),
  ('original-neighbor','packing-allocation-member-arena.bin',4),
  ('original-memory-default','packing-allocation-entry-memory.bin',0),
  ('original-memory-write','packing-allocation-return-memory.bin',4),
  ('original-global-publication','locale-packing-allocation-member-target-data.bin',0x410),
  ('original-BSS-neighbor','locale-packing-allocation-return-target-BSS.bin',0),
  ('original-snapshot-write','locale-packing-allocation-member-target-snapshot.bin',0),
  ('original-errors','locale-packing-allocation-member-errors.bin',0),
  ('original-FP','locale-packing-allocation-return-fp.bin',4),
 ]:
  data=bytearray((a.original/name).read_bytes());data[offset]^=1
  with tempfile.TemporaryDirectory(prefix='packing-alloc-control-',dir=a.original.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(data);reject(label,Overlay(a.original,{name:path}),a.native,c)
 for label,change in [('wrong-candidate-extent',lambda x:x['functions'][-1]['chunks'][0].__setitem__(1,0x134f3)),('wrong-size-contract',lambda x:x['packing_allocator_observers'].__setitem__('request_bytes',12))]:
  changed=copy.deepcopy(c);change(changed);reject(label,a.original,a.native,changed)
 result=dict(status='all affected allocation/return/receiver controls rejected',count=len(controls),controls=controls,seconds=time.perf_counter()-started,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(path.relative_to(PROJECT)):sha(path)for path in [Path(__file__),PROJECT/'scripts/packing_allocator_state.py']});a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))
if __name__=='__main__':main()
