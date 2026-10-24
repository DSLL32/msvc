#!/usr/bin/env python3
"""New Jobs/lifecycle guards against private-copy, freshly hash-bound mutations."""
import argparse,copy,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from mre_jobs_packet import decode
P=Path(__file__).resolve().parents[1];sha=lambda b:hashlib.sha256(b).hexdigest()
a=argparse.ArgumentParser(description=__doc__);a.add_argument('original',type=Path);a.add_argument('--receipt',required=True,type=Path);args=a.parse_args();start=time.perf_counter();expected=decode(args.original);passed=[]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='jobs-response-controls-')as tmp:
 d=Path(tmp)/'original';shutil.copytree(args.original,d);capture=json.loads((d/'capture.json').read_bytes())
 def reject(label,name,change):
  f=d/name;old=f.read_bytes();j=copy.deepcopy(capture)
  try:
   f.write_bytes(change(old));j['files'][name]={'bytes':f.stat().st_size,'sha256':sha(f.read_bytes())};(d/'capture.json').write_text(json.dumps(j,indent=2)+'\n')
   try:decode(d)
   except(AssertionError,ValueError,KeyError,UnicodeDecodeError,struct.error):passed.append(label)
   else:raise AssertionError('accepted '+label)
  finally:f.write_bytes(old);(d/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
 def word(offset,value=None):
  def change(b):
   x=bytearray(b);v=struct.unpack_from('<I',x,offset)[0];struct.pack_into('<I',x,offset,v^1 if value is None else value);return bytes(x)
  return change
 prefix='observer/mre-files-idb-mre-6-after-'
 for label,name,change in[
 ('wrong family','mre-observer.txt',lambda b:b.replace(b'JOBS',b'SORT')),
 ('missing call','mre-calls.txt',lambda b:b'5\n'),
 ('unknown phase','observer/mre-events.txt',lambda b:b.replace(b'snapshot phase=jobs-before ',b'snapshot phase=unknown-before ')),
 ('wrong jobs entry','observer/jobs-before-context.bin',word(184)),
 ('wrong jobs return','observer/jobs-after-context.bin',word(184)),
 ('wrong jobs savedreturn','observer/jobs-before-stack.bin',word(0)),
 ('wrong jobs actualformal','observer/jobs-before-arguments.bin',word(0,1)),
 ('wrong jobs stackedformal','observer/jobs-before-stack.bin',word(4,1)),
 ('wrong jobs cleanup','observer/jobs-after-context.bin',word(196)),
 ('wrong jobs result','observer/jobs-after-context.bin',word(176,1)),
 ('wrong jobs nonvolatile','observer/jobs-after-context.bin',word(160)),
 ('wrong release qualifiedexport','observer/mre-5-before-identity.bin',word(4)),
 ('wrong release driver','observer/mre-5-before-arguments.bin',word(0)),
 ('wrong release result','observer/mre-5-after-context.bin',word(176,0)),
 ('wrong release cleanup','observer/mre-5-after-context.bin',word(196)),
 ('changed release nonvolatile','observer/mre-5-after-context.bin',word(156)),
 ('changed release error','mktemp-mre-5-after-ptd.bin',word(8)),
 ('changed release fullFP','observer/mre-5-after-context.bin',word(120)),
 ('changed release liveposition','observer/mre-files-idb-mre-5-after-position-after.bin',word(0)),
 ('wrong close qualifiedexport','observer/mre-6-before-identity.bin',word(4)),
 ('wrong close engine','observer/mre-6-before-arguments.bin',word(0)),
 ('wrong close commit','observer/mre-6-before-arguments.bin',word(4,0)),
 ('wrong actualscan','observer/mre-6-before-first-dependency-scan.bin',word(0,0)),
 ('missing scalar','observer/mre-6-after-first-dependency-scan.bin',lambda b:b''),
 ('wrong close result','observer/mre-6-after-context.bin',word(176,0)),
 ('wrong close cleanup','observer/mre-6-after-context.bin',word(196)),
 ('wrong close nonvolatile','observer/mre-6-after-context.bin',word(164)),
 ('changed close fullFP','observer/mre-6-after-context.bin',word(124)),
 ('wrong closed schema',prefix+'request.bin',word(0,1)),
 ('wrong closed PID',prefix+'request.bin',word(28)),
 ('wrong closed slot',prefix+'request.bin',word(12)),
 ('invented closed cursor',prefix+'request.bin',word(20,1024)),
 ('wrong lastlive FileId',prefix+'last-live.bin',word(48)),
 ('wrong current FileId',prefix+'path.bin',word(48)),
 ('wrong closed lowio',prefix+'lowio-after.bin',word(4)),
 ('changed observer CONTEXT',prefix+'context-after.bin',word(164)),
 ('changed closed IDB',prefix+'contents.bin',lambda b:bytes([b[0]^1])+b[1:]),
 ('changed final IDB','case.idb',lambda b:bytes([b[0]^1])+b[1:]),
 ('wrong full completion status','events.txt',lambda b:b.replace(b'status=0\n',b'status=258\n')),
 ('wrong stdout','stdout.bin',lambda b:b'X'+b[1:])]:reject(label,name,change)
 assert decode(d)['lifecycle']==expected['lifecycle']
receipt={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'original_capture_sha256':sha((args.original/'capture.json').read_bytes()),'scope':'Freshly hashed private-copy actual Jobs/Release/Close/closed-file/completion guards; no native or app acceptance','sources':{n:sha((P/'scripts'/n).read_bytes())for n in('mre_jobs_packet.py','mre_jobs_lifecycle.py','mre_jobs_controls.py','mbslwr_sort_packet.py','dependency_response.py','check-mre-jobs-response.py')}};args.receipt.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
