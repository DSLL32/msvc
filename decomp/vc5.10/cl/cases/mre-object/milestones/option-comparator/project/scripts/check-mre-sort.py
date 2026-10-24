#!/usr/bin/env python3
"""Focused refreshed-hash guards for the first real comparator and fourth MRE call."""
import argparse,copy,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from mre_sort_packet import decode
P=Path(__file__).resolve().parents[1];sha=lambda b:hashlib.sha256(b).hexdigest()
a=argparse.ArgumentParser(description=__doc__);a.add_argument('original',type=Path);a.add_argument('--receipt',type=Path,required=True);args=a.parse_args();start=time.perf_counter();expected=decode(args.original);passed=[]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='sort-controls-')as tmp:
 d=Path(tmp)/'original';shutil.copytree(args.original,d);capture=json.loads((d/'capture.json').read_bytes())
 def reject(label,name,change):
  f=d/name;old=f.read_bytes();j=copy.deepcopy(capture)
  try:
   f.write_bytes(change(old));j['files'][name]={'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())};(d/'capture.json').write_text(json.dumps(j,indent=2)+'\n')
   try:decode(d)
   except(AssertionError,ValueError,KeyError,UnicodeDecodeError,struct.error):passed.append(label)
   else:raise AssertionError('accepted semantic mutation '+label)
  finally:f.write_bytes(old);(d/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
 def word(offset,value=None):
  def mutation(b):
   x=bytearray(b);v=struct.unpack_from('<I',x,offset)[0];struct.pack_into('<I',x,offset,v^1 if value is None else value);return bytes(x)
  return mutation
 for label,name,change in[
 ('wrong family','mre-observer.txt',lambda b:b.replace(b'SORT',b'SELECTOR')),
 ('extra call inventory','mre-calls.txt',lambda b:b'5\n'),
 ('wrong callback entry PC','observer/compare-before-context.bin',word(184)),
 ('wrong callback return PC','observer/compare-after-context.bin',word(184)),
 ('wrong qsort return word','observer/compare-before-stack.bin',word(0)),
 ('wrong callback formal word','observer/compare-before-stack.bin',word(4)),
 ('wrong callback cleanup','observer/compare-after-context.bin',word(196)),
 ('wrong callback nonvolatile','observer/compare-after-context.bin',word(164)),
 ('unnormalized comparator return','observer/compare-after-context.bin',word(176,19)),
 ('changed callback input','observer/compare-before-left.bin',lambda b:b'!'+b[1:]),
 ('changed callback owned bytes','observer/compare-after-right.bin',lambda b:b'!'+b[1:]),
 ('wrong signature frame identity','observer/compare-before-identity.bin',word(20)),
 ('wrong callback x87','observer/compare-after-context.bin',word(32)),
 ('wrong fourth qualified export','observer/mre-4-before-identity.bin',word(4)),
 ('wrong fourth caller site','observer/mre-4-before-context.bin',word(184)),
 ('wrong fourth driver argument','observer/mre-4-before-arguments.bin',word(0)),
 ('wrong fourth pending argument','observer/mre-4-before-arguments.bin',word(4)),
 ('wrong fourth result','observer/mre-4-after-context.bin',word(176,0)),
 ('wrong fourth stdcall cleanup','observer/mre-4-after-context.bin',word(196)),
 ('changed fourth nonvolatile','observer/mre-4-after-context.bin',word(160)),
 ('nonnull pending input','observer/mre-4-before-pending.bin',word(0,1)),
 ('nonnull pending output','observer/mre-4-after-pending.bin',word(16,1)),
 ('changed fourth error','mktemp-mre-4-after-ptd.bin',word(8)),
 ('changed fourth full FP','observer/mre-4-after-context.bin',word(120)),
 ('changed fourth file bytes','observer/mre-files-idb-mre-4-after-contents.bin',lambda b:bytes([b[0]^1])+b[1:]),
 ('changed fourth copy position','observer/mre-files-idb-mre-4-after-position-after.bin',word(0)),
 ('changed fourth copy context','observer/mre-files-idb-mre-4-after-context-after.bin',word(164)),
 ('wrong frontier PC','observer/mre-frontier-context.bin',word(184)),
 ('wrong frontier caller','observer/mre-frontier-stack.bin',word(0)),
 ('wrong frontier formal slot','observer/mre-frontier-stack.bin',word(4,1)),
 ('wrong frontier actual formal','observer/mre-frontier-formal-arguments.bin',word(0,1))]:reject(label,name,change)
 assert decode(d)['signature_frontier']==expected['signature_frontier']
receipt={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Private-copy refreshed rawhash new callback/API4 guards; prior unchanged API1-3/file premises remain strict; no native/app acceptance','original_capture_sha256':sha((args.original/'capture.json').read_bytes()),'sources':{n:sha((P/'scripts'/n).read_bytes())for n in('mre_sort_packet.py','mbslwr_sort_packet.py','check-mre-sort.py')}};args.receipt.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
