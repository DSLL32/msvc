#!/usr/bin/env python3
"""New tokenizer-boundary semantic denials; unchanged prefix proof is reused."""
import argparse,copy,json,struct,tempfile,time
from pathlib import Path
from environment_argument_state import original_state,validate_native_boundary
from provenance import sha

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--positive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 positive=json.loads(a.positive.read_text());project=Path(__file__).resolve().parents[1]
 assert positive['result']['execution_receipts']==dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json'))
 for name,value in positive['validator_hashes'].items():assert sha(project/'scripts'/name)==value
 candidate=json.loads((a.original/'candidate.json').read_text());entries=json.loads((a.native/'environment-entries.json').read_text());returns=json.loads((a.native/'environment-returns.json').read_text());state=json.loads((a.native/'state.json').read_text())
 validate_native_boundary(a.original,a.native,candidate,entries,returns,state)
 started=time.perf_counter();results=[]
 native_cases=[('input-nonNULL',lambda e,r,s:e[0]['args'].__setitem__('arguments',1)),('raw-stack-input',lambda e,r,s:e[0]['words'].__setitem__(1,1)),('missing-formal',lambda e,r,s:e[0].__setitem__('formal_inventory',[])),('wrong-formal-type',lambda e,r,s:e[0]['formal_inventory'][0].__setitem__('type','int')),
 ('wrong-entry-PC',lambda e,r,s:e[0].__setitem__('pc',e[0]['pc']+1)),('wrong-caller',lambda e,r,s:e[0]['caller'].__setitem__('symbol','c1_main')),('missing-call',lambda e,r,s:e.clear()),
 ('return-PC',lambda e,r,s:r[0].__setitem__('pc',0)),('cdecl-cleanup',lambda e,r,s:r[0].__setitem__('esp',r[0]['esp']+4)),('callee-save',lambda e,r,s:r[0]['preserved'].__setitem__('ebx',0)),
 ('return-state-write',lambda e,r,s:r[0]['compiler_state']['bss'].__setitem__('command_argc',0)),('full-FP-status',lambda e,r,s:r[0]['fp'].__setitem__('sw',0)),
 ('warning-default',lambda e,r,s:s['compiler_state']['option_state']['initialized']['warning_levels'].__setitem__(611,0)),('warning-neighbor',lambda e,r,s:s['compiler_state']['option_state']['initialized']['warning_levels'].__setitem__(612,1)),
 ('next-count',lambda e,r,s:s['args'].__setitem__('count',41)),('next-mode',lambda e,r,s:s['args'].__setitem__('mode',0)),('omitted-next-formal',lambda e,r,s:s['formal_inventory'].pop()),('wrong-next-PC',lambda e,r,s:s.__setitem__('pc',0))]
 for label,mutate in native_cases:
  e,r,s=copy.deepcopy((entries,returns,state));mutate(e,r,s);begin=time.perf_counter()
  try:validate_native_boundary(a.original,a.native,candidate,e,r,s)
  except AssertionError:pass
  else:raise AssertionError('accepted native '+label)
  results.append(dict(side='native',control=label,seconds=time.perf_counter()-begin))
 original_cases=[('entry-registers','environment-entry-registers.bin',9*4,1),('return-address','environment-return-registers.bin',4,0),('return-cleanup','environment-return-registers.bin',8,0),('callee-save','environment-return-registers.bin',4*4,0),('entry-FP','environment-entry-fp.bin',4,0),('unknown-data-write','environment-return-target-data.bin',0,None),('BSS-neighbor-write','environment-return-target-BSS.bin',0,None),('snapshot-write','environment-return-target-snapshot.bin',0,None)]
 for label,name,offset,value in original_cases:
  with tempfile.TemporaryDirectory(prefix='env-boundary-negative-',dir=a.original.parent)as tmp:
   d=Path(tmp)
   for f in a.original.iterdir():
    if f.is_file():(d/f.name).symlink_to(f)
   raw=bytearray((d/name).read_bytes())
   if value is None:raw[offset]^=1
   else:struct.pack_into('<I',raw,offset,value)
   (d/name).unlink();(d/name).write_bytes(raw);begin=time.perf_counter()
   try:original_state(d,candidate)
   except AssertionError:pass
   else:raise AssertionError('accepted original '+label)
   results.append(dict(side='original',control=label,seconds=time.perf_counter()-begin))
 result=dict(status='PASS',scope='fresh affected tokenizer ABI/state/caller/whole-state checks; unchanged full-prefix controls reused',positive_sha256=sha(a.positive),execution_receipts=positive['result']['execution_receipts'],checker_sha256=sha(project/'scripts/environment_argument_state.py'),controller_sha256=sha(Path(__file__)),controls=results,seconds=time.perf_counter()-started)
 assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(results),seconds=result['seconds'])))
if __name__=='__main__':main()
