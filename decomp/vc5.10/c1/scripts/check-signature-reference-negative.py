#!/usr/bin/env python3
"""Focused rehashed semantic denials for the one external response observation."""
import argparse,json,struct,tempfile,time,importlib.util
from pathlib import Path
from provenance import sha
spec=importlib.util.spec_from_file_location('reference',Path(__file__).with_name('check-signature-reference.py'));reference=importlib.util.module_from_spec(spec);spec.loader.exec_module(reference)

def main():
 p=argparse.ArgumentParser();p.add_argument('original',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();baseline=reference.check(a.original)
 cases=[('analysis-role',None,None,None),('accepted-nextarg',None,None,None),('provider-identity','loaded-mspdb50.bin',100,None),('return-PC','signature-reference-return-registers.bin',4,0),('RET0','signature-reference-return-registers.bin',8,0),('nonvolatile','signature-reference-return-registers.bin',7*4,0),('byte-count','signature-reference-return-registers.bin',13*4,2),('mutated-private-slot','signature-reference-return-registers.bin',14*4,0xffffffff),('return-value','signature-reference-return-registers.bin',3*4,0),('complete-input','signature-reference-entry-input-000.bin',0,None),('full-FP','locale-signature-return-fp.bin',4,None),('readonly-table','signature-provider-table-after-000.bin',17,None),('caller-discard', 'relocated-text.bin',0x12c33-0x1000,None),('neighbor-write',None,None,None)]
 started=time.perf_counter();controls=[]
 for label,name,offset,value in cases:
  with tempfile.TemporaryDirectory(prefix='signature-reference-negative-',dir=a.original.parent)as tmp:
   d=Path(tmp)
   for f in a.original.iterdir():
    if f.is_file():(d/f.name).symlink_to(f)
   receipt=json.loads((d/'receipt.json').read_text())
   def replace(n,data):
    f=d/n;f.unlink();f.write_bytes(data);receipt['artifacts'][n]=dict(size=len(data),sha256=sha(f))
   if name:
    data=bytearray((d/name).read_bytes())
    if value is None:data[offset]^=1
    else:struct.pack_into('<I',data,offset,value)
    replace(name,data)
   elif label=='analysis-role':receipt['kind']='explicit-original-capture'
   elif label=='accepted-nextarg':
    c=json.loads((d/'functions.json').read_text());c['accepted'].append('_next_argument');replace('functions.json',(json.dumps(c)+'\n').encode())
   else:
    events=(d/'events.txt').read_text();import re
    sequence=int(re.findall(r'^trap seq=(\d+) rva=00012f9c ',events,re.M)[0]);n='compiler-bss-%03d.bin'%sequence;data=bytearray((d/n).read_bytes());data[0]^=1;replace(n,data)
   f=d/'receipt.json';f.unlink();f.write_text(json.dumps(receipt)+'\n');begin=time.perf_counter()
   try:reference.check(d)
   except AssertionError as error:
    assert not str(error).startswith('changed artifact '),'mutation failed integrity instead of semantic check'
   else:raise AssertionError('accepted '+label)
   controls.append(dict(control=label,seconds=time.perf_counter()-begin))
 result=dict(status='PASS',scope='new exact public response/changed private slot/dead caller slot proof; no execution',original_receipt_sha256=sha(a.original/'receipt.json'),baseline=baseline,checker_sha256=sha(Path(__file__).with_name('check-signature-reference.py')),controller_sha256=sha(Path(__file__)),controls=controls,seconds=time.perf_counter()-started)
 assert not a.output.exists();a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status='PASS',controls=len(controls),seconds=result['seconds'])))
if __name__=='__main__':main()
