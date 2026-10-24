#!/usr/bin/env python3
"""New vector/ownership/dependency-boundary denials, reusing validated prefix."""
import argparse,copy,json,struct,tempfile,time
from pathlib import Path
from vector_state import validate_allocation_state,snapshot
from frontier_contract import validate_original_frontier
from provenance import sha

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('native',type=Path);p.add_argument('--positive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();project=Path(__file__).resolve().parents[1]
 positive=json.loads(a.positive.read_text());assert positive['result']['execution_receipts']==dict(original=sha(a.original/'receipt.json'),native=sha(a.native/'receipt.json'))
 for name,value in positive['validator_hashes'].items():assert sha(project/'scripts'/name)==value
 c=json.loads((a.original/'candidate.json').read_text());e=json.loads((a.native/'vector-entries.json').read_text());r=json.loads((a.native/'vector-returns.json').read_text());s=json.loads((a.native/'state.json').read_text());first=snapshot(a.original,a.native,s,e[0]);validate_allocation_state(a.original,a.native,c,e,r,s,first)
 cases=[('missing-call',lambda e,r,s:e.pop()),('wrong-argument',lambda e,r,s:e[1]['args'].__setitem__('count',41)),('wrong-mode',lambda e,r,s:e[1]['words'].__setitem__(2,2)),('missing-formal',lambda e,r,s:e[2]['formal_inventory'].pop()),('wrong-layout',lambda e,r,s:e[0]['layout']['offsets'].__setitem__('items',12)),('return-PC',lambda e,r,s:r[0].__setitem__('pc',0)),('return-cleanup',lambda e,r,s:r[0].__setitem__('esp',0)),('callee-save',lambda e,r,s:r[0]['preserved'].__setitem__('ebx',0)),('header-count',lambda e,r,s:r[1]['header'].__setitem__(0,1)),('header-capacity',lambda e,r,s:r[1]['header'].__setitem__(1,39)),('header-arena',lambda e,r,s:r[1]['header'].__setitem__(2,0)),('items-owner',lambda e,r,s:r[2]['header'].__setitem__(3,r[0]['header'][3])),('items-content',lambda e,r,s:r[0]['items'].__setitem__(17,1)),('cursor-write',lambda e,r,s:r[1]['compiler_state']['bss']['arenas'][1].__setitem__('cursor',0)),('neighbor-write',lambda e,r,s:r[1]['compiler_state']['bss'].__setitem__('command_argc',0)),('FP-state',lambda e,r,s:r[2]['fp'].__setitem__('sw',0)),('errno',lambda e,r,s:r[0]['errors'].__setitem__(0,1)),('last-published-pointer',lambda e,r,s:s['compiler_state']['option_state']['bss'].__setitem__('forced_includes',0)),('dependency-arg-count',lambda e,r,s:s['args'].__setitem__('byte_count',2)),('dependency-seed',lambda e,r,s:s['args'].__setitem__('signature',0)),('dependency-input',lambda e,r,s:s['dependency_entry']['input'].__setitem__(0,68)),('dependency-raw-stack',lambda e,r,s:s['dependency_entry']['stack'].__setitem__(2,2)),('dependency-formals',lambda e,r,s:s['formal_inventory'].pop()),('dependency-real-PC',lambda e,r,s:s.__setitem__('pc',0))]
 started=time.perf_counter();controls=[]
 for label,mutate in cases:
  x,y,z=copy.deepcopy((e,r,s));mutate(x,y,z);before=time.perf_counter()
  try:validate_allocation_state(a.original,a.native,c,x,y,z,first)
  except AssertionError:pass
  else:raise AssertionError('accepted native '+label)
  controls.append(dict(side='native',control=label,seconds=time.perf_counter()-before))
 raw_cases=[('full-owned-header','vector-header-001.bin',2,None),('full-owned-items','vector-items-002.bin',159,None),('committed-neighbor','vector-committed-002.bin',32767,None),('commit-zero-prefix','vector-commit-zero-000.bin',32767,None),('entry-caller','vector-0-entry-registers.bin',32,0),('entry-raw-mode','vector-0-entry-registers.bin',40,0),('return-cleanup','vector-0-return-registers.bin',8,0),('method-receiver','vector-member-entry-registers.bin',32,0),('method-return','vector-member-return-registers.bin',4,0),('public-input','signature-input-000.bin',0,None)]
 for label,name,offset,value in raw_cases:
  with tempfile.TemporaryDirectory(prefix='vector-negative-',dir=a.original.parent)as tmp:
   d=Path(tmp)
   for f in a.original.iterdir():
    if f.is_file():(d/f.name).symlink_to(f)
   raw=bytearray((d/name).read_bytes())
   if value is None:raw[offset]^=1
   else:struct.pack_into('<I',raw,offset,value)
   (d/name).unlink();(d/name).write_bytes(raw);before=time.perf_counter()
   try:validate_allocation_state(d,a.native,c,e,r,s,first)
   except AssertionError:pass
   else:raise AssertionError('accepted original '+label)
   controls.append(dict(side='original',control=label,seconds=time.perf_counter()-before))
 for label,probe,events in [('normal-as-target','normal','frontier rva=00012f9c\n'),('restore-as-provider','restore','dependency-frontier module=MSPDB50.dll rva=0000f22d seq=281\n'),('unknown-provider','normal','dependency-frontier module=UNKNOWN.dll rva=0000f22d seq=281\n')]:
  try:validate_original_frontier(events,c,probe)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  controls.append(dict(side='role',control=label))
 result=dict(status='PASS',scope='new vector/known-state/owned-commit/qualified-dependency roles; unchanged full prefix controls reused',positive_sha256=sha(a.positive),execution_receipts=positive['result']['execution_receipts'],checker_sha256=sha(project/'scripts/vector_state.py'),controller_sha256=sha(Path(__file__)),controls=controls,seconds=time.perf_counter()-started)
 assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(controls),seconds=result['seconds'])))
if __name__=='__main__':main()
