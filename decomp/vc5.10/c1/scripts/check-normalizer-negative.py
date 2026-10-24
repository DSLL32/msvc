#!/usr/bin/env python3
"""Fresh leaf/public-ABI/CWD/span and parser-state controls; no executions."""
import argparse,json,re,tempfile,time
from pathlib import Path
from normalizer_state import validate_sequence as validate_sequence_impl
from vector_state import snapshot
from provenance import PROJECT,sha

class Overlay:
 def __init__(self,base,files):self.base,self.files=base,files
 def __truediv__(self,name):return self.files.get(name,self.base/name)
 def resolve(self):return self.base.resolve()

def validate_sequence(original,native,candidate):
 if candidate['expected_frontier']['kind']=='packing-initializer-allocator':
  state=json.loads((native/'state.json').read_text());rows=json.loads((native/'packing-entries.json').read_text());assert len(rows)==1
  return validate_sequence_impl(original,native,candidate,stage_state=snapshot(original,native,state,rows[0]))
 return validate_sequence_impl(original,native,candidate)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 c=json.loads((a.original/'candidate.json').read_text());started=time.perf_counter();validate_sequence(a.original,a.native,c);controls=[]
 def reject(label,scope,op):
  try:op()
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(control=label,scope=scope))
 def native(label,name,change):
  if c['expected_frontier']['kind']=='packing-initializer-allocator'and name=='state.json':
   name='packing-entries.json';data=json.loads((a.native/name).read_text());change(data[0])
  else:data=json.loads((a.native/name).read_text());change(data)
  with tempfile.TemporaryDirectory(prefix='normalizer-control-',dir=a.native.parent)as tmp:
   path=Path(tmp)/name;path.write_text(json.dumps(data));overlay=Overlay(a.native,{name:path});reject(label,'native',lambda:validate_sequence(a.original,overlay,c))
 for label,name,change in [
  ('second-call-omission','normalizer-entries.json',lambda x:x.pop()),
  ('entry-formal-omission','normalizer-entries.json',lambda x:x[0]['formal_inventory'].clear()),
  ('entry-physical-PC','normalizer-entries.json',lambda x:x[0].__setitem__('pc',0)),
  ('second-output-role','normalizer-entries.json',lambda x:x[1]['args'].__setitem__('path',0)),
  ('low-byte-fold','normalizer-returns.json',lambda x:x[1]['path_bytes'].__setitem__(0,ord('S'))),
  ('original-pointer-return','normalizer-returns.json',lambda x:x[0].__setitem__('eax',0)),
  ('normalizer-RET-cleanup','normalizer-returns.json',lambda x:x[1].__setitem__('esp',0)),
  ('normalizer-callee-save','normalizer-returns.json',lambda x:x[0]['preserved'].__setitem__('edi',0)),
  ('native-private-slot-record','normalizer-returns.json',lambda x:x[0].__setitem__('argument_slot_after',0)),
  ('public-call-omission','fullpath-entries.json',lambda x:x.clear()),
  ('public-formal-omission','fullpath-entries.json',lambda x:x[0]['formal_inventory'].pop()),
  ('public-capacity','fullpath-entries.json',lambda x:x[0]['args'].__setitem__('capacity',519)),
  ('public-input-NUL','fullpath-entries.json',lambda x:x[0]['input_bytes'].__setitem__(-1,1)),
  ('public-entry-identity','fullpath-entries.json',lambda x:x[0].__setitem__('pc',0)),
  ('public-result-alias','fullpath-returns.json',lambda x:x[0].__setitem__('eax',0)),
  ('public-output-path','fullpath-returns.json',lambda x:x[0]['output_bytes'].__setitem__(0,ord('Z'))),
  ('public-callee-save','fullpath-returns.json',lambda x:x[0]['preserved'].__setitem__('esi',x[0]['preserved']['esi']^1)),
  ('public-errors','fullpath-returns.json',lambda x:x[0]['errors'].__setitem__(0,1)),
  ('public-FP','fullpath-returns.json',lambda x:x[0]['fp'].__setitem__('cw',0x37f)),
  ('logical-cwd','fullpath-entries.json',lambda x:x[0].__setitem__('logical_cwd','S:\\wrong\\')),
  ('physical-cwd','fullpath-returns.json',lambda x:x[0].__setitem__('physical_cwd',str(a.original))),
  ('private-root-fallback','native-path-binding.json',lambda x:x.__setitem__('private_S_root',str(a.original))),
  ('input-provenance','native-path-binding.json',lambda x:x['inputs'][0].__setitem__('sha256','0'*64)),
  ('R-repo-mapping','native-path-binding.json',lambda x:x.__setitem__('R_root','/tmp')),
  ('parser-path-output','state.json',lambda x:x['compiler_state']['option_state']['initialized']['database_path'].__setitem__(0,0)),
  ('parser-c9-il','state.json',lambda x:x['compiler_state']['initialized'].__setitem__('c9_il',0)),
  ('parser-function-records','state.json',lambda x:x['compiler_state']['option_state']['initialized'].__setitem__('function_records',1)),
  ('parser-line-records','state.json',lambda x:x['compiler_state']['option_state']['initialized'].__setitem__('line_records',1)),
  ('parser-current-PDB','state.json',lambda x:x['compiler_state']['option_state'].__setitem__('current_pdb',0)),
  ('snapshot-neighbor','state.json',lambda x:x['compiler_state']['option_state']['snapshot'].__setitem__('packing_stack',1)),
  ('packing-formal-omission','state.json',lambda x:x['formal_inventory'].append(dict(name='fake',type='int'))),
  ('packing-physical-trap','state.json',lambda x:x.__setitem__('pc',x['pc']-1)),
 ]:native(label,name,change)
 seq=int(re.findall(r'^trap seq=(\d+) rva=0001348d ',(a.original/'events.txt').read_text(),re.M)[0])
 for label,side,name,offset in [
  ('original-normalizer-slot',a.original,'normalizer-0-return-registers.bin',56),
  ('original-normalizer-return',a.original,'normalizer-1-return-registers.bin',12),
  ('original-fullpath-capacity',a.original,'fullpath-entry-registers.bin',52),
  ('original-fullpath-result',a.original,'fullpath-return-registers.bin',12),
  ('original-CWD-length',a.original,'fullpath-entry-cwd-identity.bin',12),
  ('original-CWD-character',a.original,'fullpath-return-cwd.bin',6),
  ('original-public-errors',a.original,'locale-fullpath-return-errors.bin',0),
  ('original-public-FP',a.original,'locale-fullpath-return-fp.bin',4),
  ('original-public-locale-table',a.original,'locale-fullpath-return-ctype.bin',200),
  ('original-output-tail',a.original,'fullpath-return-buffer.bin',519),
  ('native-output-tail',a.native,'fullpath-native-return-buffer.bin',519),
  ('original-parser-PDB',a.original,'state-%03d.bin'%seq,0x68),
 ]:
  data=bytearray((side/name).read_bytes());data[offset]^=1
  with tempfile.TemporaryDirectory(prefix='normalizer-control-',dir=side.parent)as tmp:
   path=Path(tmp)/name;path.write_bytes(data);overlay=Overlay(side,{name:path})
   reject(label,'original'if side==a.original else'native',lambda:validate_sequence(overlay if side==a.original else a.original,overlay if side==a.native else a.native,c))
 result=dict(status='all affected normalizer/fullpath controls rejected',count=len(controls),seconds=time.perf_counter()-started,controls=controls,execution_receipts=dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json')),validators={str(p):sha(p)for p in [Path(__file__),PROJECT/'scripts/normalizer_state.py']})
 a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(count=result['count'],seconds=result['seconds'])))

if __name__=='__main__':main()
