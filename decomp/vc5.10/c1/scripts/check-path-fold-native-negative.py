#!/usr/bin/env python3
"""Reuse an unchanged validated prefix; mutate only the new scalar API proof."""
import argparse,copy,json,time
from pathlib import Path
from path_fold_state import validate_calls
from path_fold_original import check as check_original
from provenance import sha,captured_native_binary

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('native',type=Path);parser.add_argument('--positive',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
 original=args.original.resolve();native=args.native.resolve();positive=json.loads(args.positive.read_text());project=Path(__file__).resolve().parents[1]
 assert positive['result']['execution_receipts']==dict(original=sha(original/'receipt.json'),native=sha(native/'receipt.json'))
 for name,digest in positive['validator_hashes'].items():assert sha(project/'scripts'/name)==digest
 for directory in (original,native):
  receipt=json.loads((directory/'receipt.json').read_text())
  for name,item in receipt['artifacts'].items():
   path=directory/name;assert path.is_file()and path.stat().st_size==item['size']and sha(path)==item['sha256']
 own=check_original(original);binary=captured_native_binary(native);calls=json.loads((native/'path-fold-calls.json').read_text());validate_calls(original,native,calls,binary,own)
 cases=[('argument',lambda x:x['entry']['words'].__setitem__(1,256)),('omitted-formal',lambda x:x['entry'].__setitem__('formal_inventory',[])),
  ('signed-formal',lambda x:x['entry']['formal_inventory'][0].__setitem__('type','int')),('caller',lambda x:x['entry']['caller'].__setitem__('symbol','parse_command_line')),
  ('callsite',lambda x:x['entry']['words'].__setitem__(0,x['entry']['words'][0]+1)),('RET-address',lambda x:x['return'].__setitem__('pc',0)),
  ('RET-cleanup',lambda x:x['return'].__setitem__('esp',x['return']['esp']+4)),('full-unsigned-result',lambda x:x['return'].__setitem__('eax',x['return']['eax']+65536)),
  ('callee-save',lambda x:x['return']['preserved'].__setitem__('ebp',0)),('errno',lambda x:x['return']['errors'].__setitem__(0,1)),
  ('locale',lambda x:x['entry'].__setitem__('lc_codepage',932)),('MB-codepage',lambda x:x['entry'].__setitem__('mb_codepage',0)),
  ('full-MB-table',lambda x:x['entry']['mb'].__setitem__(1,x['entry']['mb'][1]^1)),('current-ctype',lambda x:x['entry']['ctype'].__setitem__(2,x['entry']['ctype'][2]^1)),
  ('retained-ctype',lambda x:x['entry']['old_ctype'].__setitem__(2,x['entry']['old_ctype'][2]^1)),('result-string',lambda x:x['entry']['result'].__setitem__(0,0)),
  ('output-progress',lambda x:x['entry']['fold'].__setitem__(0,17)),('input-scanner',lambda x:x['entry']['classes'].__setitem__(0,35)),
  ('owner-alias',lambda x:x['entry']['addresses'].__setitem__('ctype',x['entry']['addresses']['old_ctype'])),('binding-reference',lambda x:x['entry']['selection'].__setitem__('reference','0'*64)),
  ('selection-count',lambda x:x['entry']['selection'].__setitem__('mb_calls',0)),('floating-status',lambda x:x['entry']['fp'].__setitem__('sw',0))]
 started=time.perf_counter();results=[]
 for label,mutate in cases:
  modified=list(calls);modified[128]=copy.deepcopy(calls[128]);mutate(modified[128]);before=time.perf_counter()
  try:validate_calls(original,native,modified,binary,own)
  except AssertionError:pass
  else:raise AssertionError('accepted '+label)
  results.append(dict(control=label,seconds=time.perf_counter()-before))
 result=dict(status='PASS',kind='new API component controls with exact hash-verified complete-prefix reuse; no target execution',positive_sha256=sha(args.positive),native_receipt_sha256=sha(native/'receipt.json'),original_receipt_sha256=sha(original/'receipt.json'),checker_sha256=sha(Path(__file__).with_name('path_fold_state.py')),controller_sha256=sha(Path(__file__)),controls=results,seconds=time.perf_counter()-started)
 assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(results),seconds=result['seconds'])))

if __name__=='__main__':main()
