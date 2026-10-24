#!/usr/bin/env python3
"""Exact Jobs-family admission and owned-cache integrity; no acceptance credit."""
import copy,hashlib,json,shutil,tempfile,time
from pathlib import Path
from accept import reconstruction_capture
from mre_native import Provider
P=Path(__file__).resolve().parents[1];base=Path('/root/.ai-tmp/msvc2/vc5.10/cl');raw=json.loads((base/'jobs-1-original/capture.json').read_bytes());reconstruction_capture(raw);passed=[];start=time.perf_counter()
for name,change in[
 ('baseline',lambda c:c.__setitem__('baseline',True)),('wrong case',lambda c:c.__setitem__('case','minimal-c')),
 ('analysis flag',lambda c:c.__setitem__('mre_analysis',True)),('other dependency',lambda c:c.__setitem__('dependency_response',True)),
 ('unmasked',lambda c:c['command'].__setitem__(-2,'0')),('unknown marker',lambda c:c['files'].__setitem__('mre-observer.txt',{'bytes':1,'sha256':'0'*64})),
 ('helper failure',lambda c:c.__setitem__('controller_error','incomplete lifecycle'))]:
 c=copy.deepcopy(raw);change(c)
 try:reconstruction_capture(c)
 except AssertionError:passed.append(name)
 else:raise AssertionError('admitted '+name)
with tempfile.TemporaryDirectory(dir=base,prefix='jobs-cache-controls-')as tmp:
 d=Path(tmp);ref=P/'cases/mre-object/reference';owned=d/'reference';shutil.copytree(ref,owned)
 try:Provider(owned,d,jobs_candidate=False,cached=True)
 except AssertionError:passed.append('cache requires exact Jobs family')
 else:raise AssertionError('cache admitted for other family')
 for label,n in [('owned initial IDB','observer/mre-files-idb-mre-1-after-contents.bin'),('owned final IDB','observer/mre-files-idb-mre-6-after-contents.bin'),('owned namespace','environment.bin'),('owned oracle','packet.json')]:
  f=owned/n;old=f.read_bytes()
  try:
   f.write_bytes(bytes([old[0]^1])+old[1:])
   try:Provider(owned,d,jobs_candidate=True,cached=True)
   except(AssertionError,ValueError):passed.append(label)
   else:raise AssertionError('changed cache accepted '+label)
  finally:f.write_bytes(old)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();report={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'sources':{n:sha(P/'scripts'/n)for n in('check-mre-jobs-admission.py','accept.py','mre_native.py')},'cached_contract_sha256':sha(P/'cases/mre-object/reference/cached-contract.json')};(base/'jobs-response/admission-controls.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
