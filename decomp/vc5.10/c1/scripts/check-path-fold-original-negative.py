#!/usr/bin/env python3
"""Focused semantic mutations of newly encountered path/API evidence only."""
import argparse,copy,json,os,struct,tempfile,time
from pathlib import Path
from path_fold_original import check
from provenance import sha

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
 original=args.original.resolve();baseline=check(original);assert baseline['probe']=='normal'
 started=time.perf_counter();results=[]
 cases=[('missing-call',None,None,None),('unsigned-argument','entry',11,1),('full-result-highbits','return',5,65536),
  ('return-pc','return',3,0x10612f68),('cdecl-cleanup','return',4,0x12345),('cached-ebp','return',9,0),
  ('errno','return',12,1),('locale-codepage','entry',20+2,932),('full-MB-table','entry',264+1,None),
  ('current-ctype','entry',777+2,None),('retained-ctype','entry',1291+2,None),('result-lifetime','entry',1805,None),
  ('provider-map-lifetime','both',521+200,None),('partial-table','entry',1869+2,None),('full-scanner-input','entry',2381+2,None),
  ('slash-output',None,None,None),('unknown-initialized-write',None,None,None),('BSS-neighbor-write',None,None,None),('observer-extent',None,None,None)]
 for label,phase,index,value in cases:
  with tempfile.TemporaryDirectory(prefix='path-fold-control-',dir=original.parent)as temporary:
   directory=Path(temporary)
   for path in original.iterdir():
    if path.is_file():(directory/path.name).symlink_to(path)
   receipt=json.loads((original/'receipt.json').read_text())
   def replace(name,raw):
    path=directory/name;path.unlink();path.write_bytes(raw);receipt['artifacts'][name]=dict(size=len(raw),sha256=sha(path))
   if phase:
    phases=['entry','return']if phase=='both'else[phase]
    for current in phases:
     name='path-fold-%03d-%s.bin'%(1 if phase=='both'else 0,current);raw=bytearray((directory/name).read_bytes())
     if value is None:raw[index]^=1
     else:struct.pack_into('<I',raw,index*4,value)
     replace(name,raw)
   elif label=='missing-call':
    name='path-fold-255-entry.bin';(directory/name).unlink();receipt['artifacts'].pop(name)
   elif label=='observer-extent':
    name='candidate.json';data=json.loads((directory/name).read_text());data['path_fold_observers']['table_bytes']=514;replace(name,(json.dumps(data)+'\n').encode())
   else:
    name={'slash-output':'path-fold-complete-target-BSS.bin','unknown-initialized-write':'path-fold-complete-target-data.bin','BSS-neighbor-write':'path-fold-complete-target-BSS.bin'}[label]
    raw=bytearray((directory/name).read_bytes());offset={'slash-output':0x5928+47*2,'unknown-initialized-write':0,'BSS-neighbor-write':0x5927}[label];raw[offset]^=1;replace(name,raw)
   path=directory/'receipt.json';path.unlink();path.write_text(json.dumps(receipt,indent=2)+'\n')
   before=time.perf_counter()
   try:check(directory)
   except AssertionError as error:
    message=str(error);assert not message.startswith('original path fold: artifact '),'mutation did not reach the semantic check'
   else:raise AssertionError('accepted '+label)
   results.append(dict(control=label,rejection=message,seconds=time.perf_counter()-before))
 result=dict(status='PASS',scope='new path-table/public-API semantic controls only; no original/native execution',original_receipt_sha256=sha(original/'receipt.json'),checker_sha256=sha(Path(__file__).with_name('path_fold_original.py')),controller_sha256=sha(Path(__file__)),baseline_validation=baseline,controls=results,seconds=time.perf_counter()-started)
 assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(results),seconds=result['seconds'])))

if __name__=='__main__':main()
