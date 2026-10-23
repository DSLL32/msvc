#!/usr/bin/env python3
"""Check the fast independent PE oracle against its previous scalar algorithm."""
import importlib.util,json,random,struct,time,hashlib
from pathlib import Path
BINARY=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('editor_artifacts',Path(__file__).with_name('artifacts-editor.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def scalar(data):
 image=bytearray(data);_,optional=m.layout(image)
 image[optional+64:optional+68]=bytes(4)
 if len(image)&1:image.append(0)
 value=0
 for offset in range(0,len(image),2):
  value+=struct.unpack_from('<H',image,offset)[0]
  value=(value&65535)+(value>>16)
 value=(value&65535)+(value>>16)
 return value+len(data)
base=BINARY/'cases/editor-rebase-bind';seed=(base/'inputs/client.exe').read_bytes()[:512]
rng=random.Random(0x100);bounded=[]
for n in (512,513,514,515,1023,1024,3071,3072,4095,4096,4097):
 for kind in ('zero','carry','random'):
  tail=(bytes(n-512) if kind=='zero' else b'\xff'*(n-512) if kind=='carry' else rng.randbytes(n-512))
  data=seed+tail;actual=m.checksum(data);assert actual==scalar(data),(n,kind)
  bounded.append(dict(bytes=n,kind=kind,checksum=actual))
# This includes every actual2MB positive stage and the original compact defect.
paths=[base/'inputs/client.exe',base/'inputs/provider.dll']
paths += [base/'expected'/stage/'client.exe' for stage in ('rebase','bind','release')]
paths += [base/'capture/compact-input-probe/release-1/client.exe',base/'capture/compact-input-probe/inputs/bound/client.exe']
rows=[]
for p in paths:
 data=p.read_bytes();start=time.perf_counter();old=scalar(data);old_seconds=time.perf_counter()-start
 start=time.perf_counter();new=m.checksum(data);new_seconds=time.perf_counter()-start
 assert new==old
 rows.append(dict(path=str(p.relative_to(BINARY)),sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),checksum=new,scalar_seconds=old_seconds,array_seconds=new_seconds))
for stage in ('rebase','bind','release'):
 data=(base/'expected'/stage/'client.exe').read_bytes();_,optional=m.layout(data)
 assert struct.unpack_from('<I',data,optional+64)[0]==scalar(data)
 assert m.timestamp_fields(data,'edited-pe')==[dict(offset=m.layout(data)[0]+8,bytes=4,name='PE COFF TimeDateStamp')]
compact=(base/'capture/compact-input-probe/release-1/client.exe').read_bytes();_,optional=m.layout(compact)
assert struct.unpack_from('<I',compact,optional+64)[0]==scalar(compact[:3072])!=scalar(compact)
assert m.timestamp_fields(compact,'compact-release-defect')==[] and m.normalize_artifact(compact,'compact-release-defect')==compact
print(json.dumps(dict(status='pass',bounded=bounded,actual_images=rows,original_executions=0,scope='Independent artifact oracle only; reconstruction checksum C unchanged'),indent=2))
