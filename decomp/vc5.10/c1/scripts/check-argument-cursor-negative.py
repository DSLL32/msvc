#!/usr/bin/env python3
"""Focused iterator/formal/borrowed-storage controls; prefix proof is reused."""
import argparse,copy,json,struct,tempfile,time
from pathlib import Path
from argument_cursor_state import validate_boundary
from provenance import sha

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--positive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();project=Path(__file__).resolve().parents[1]
 positive=json.loads(a.positive.read_text());assert positive['result']['candidate_executed']is True
 assert positive['result']['execution_receipts']==dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json'))
 for name,value in positive['validator_hashes'].items():assert sha(project/'scripts'/name)==value
 c=json.loads((a.original/'candidate.json').read_text());e=json.loads((a.native/'argument-cursor-entries.json').read_text())[0];r=json.loads((a.native/'argument-cursor-returns.json').read_text())[0];s=json.loads((a.native/'state.json').read_text())
 validate_boundary(a.original,a.native,c,e,r,s)
 mutations=[('extra-formal',lambda e,r,s:e['formal_inventory'].append(dict(name='fake',type='int'))),('entry-PC',lambda e,r,s:e.__setitem__('pc',0)),('caller-return',lambda e,r,s:e['words'].__setitem__(0,0)),('return-PC',lambda e,r,s:r.__setitem__('pc',0)),('return-cleanup',lambda e,r,s:r.__setitem__('esp',0)),('callee-save',lambda e,r,s:r['preserved'].__setitem__('ebx',0)),('borrowed-result-identity',lambda e,r,s:r.__setitem__('eax',r['eax']+1)),('returned-NUL',lambda e,r,s:r['result_bytes'].__setitem__(3,1)),('argc-decrement',lambda e,r,s:r['compiler_state']['bss'].__setitem__('command_argc',11)),('argv-pointer-step',lambda e,r,s:r['compiler_state']['bss'].__setitem__('command_argv',0)),('neighbor-field',lambda e,r,s:r['compiler_state']['bss'].__setitem__('debug_break',1)),('FP-state',lambda e,r,s:r['fp'].__setitem__('sw',0)),('errors',lambda e,r,s:r['errors'].__setitem__(0,1)),('matcher-formal-omission',lambda e,r,s:s['formal_inventory'].pop()),('callback-identity',lambda e,r,s:s['args'].__setitem__('next',0)),('rules-identity',lambda e,r,s:s['args'].__setitem__('rules',0)),('matcher-copy',lambda e,r,s:s['args'].__setitem__('copy',1))]
 controls=[];started=time.perf_counter()
 for label,mutate in mutations:
  x,y,z=copy.deepcopy((e,r,s));mutate(x,y,z)
  try:validate_boundary(a.original,a.native,c,x,y,z)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(side='native',control=label))
 for label,name,offset in [('raw-return-EAX','argument-cursor-return-registers.bin',12),('raw-return-ESP','argument-cursor-return-registers.bin',8),('raw-callee-save','argument-cursor-return-registers.bin',16),('raw-result','argument-cursor-result-000.bin',0),('BSS-neighbor','argument-cursor-return-target-BSS.bin',0),('snapshot-neighbor','argument-cursor-return-target-snapshot.bin',0),('data-neighbor','argument-cursor-return-target-data.bin',0)]:
  with tempfile.TemporaryDirectory(prefix='iterator-negative-',dir=a.original.parent)as tmp:
   directory=Path(tmp)
   for f in a.original.iterdir():
    if f.is_file():(directory/f.name).symlink_to(f)
   raw=bytearray((directory/name).read_bytes());raw[offset]^=1;(directory/name).unlink();(directory/name).write_bytes(raw)
   try:validate_boundary(directory,a.native,c,e,r,s)
   except AssertionError:pass
   else:raise AssertionError('accepted '+label)
  controls.append(dict(side='original',control=label))
 result=dict(status='PASS',controls=controls,seconds=time.perf_counter()-started,positive_sha256=sha(a.positive),execution_receipts=positive['result']['execution_receipts'],controller_sha256=sha(Path(__file__)),checker_sha256=sha(project/'scripts/argument_cursor_state.py'),scope='new iterator/body/ABI/state and matcher formals; existing core controls retained/reused')
 assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(controls),seconds=result['seconds'])))
if __name__=='__main__':main()
