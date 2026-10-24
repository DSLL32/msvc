#!/usr/bin/env python3
"""Affected actual six-call lifecycle/Jobs/exit controls; raw restored sequentially."""
import argparse,copy,hashlib,json,struct,time
from pathlib import Path
from mre_state import verify
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True);a=p.parse_args();passed=[];start=time.perf_counter()
SOURCES=('check-mre-jobs-native.py','mre_jobs_packet.py','mre_jobs_lifecycle.py','mre_native.py','mre_state.py','sort_native_state.py','jobs_native_state.py','mre_elf.py')
project=Path(__file__).resolve().parents[1];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();frozen_sources={n:sha(project/'scripts'/n)for n in SOURCES}
run=lambda:verify(a.original,a.native,jobs_candidate=True)
run()
def negative(label,name,mutate,raw=False):
 f=a.native/name;old=f.read_bytes()
 try:
  if raw:f.write_bytes(mutate(old))
  else:
   r=json.loads(old);mutate(r);f.write_text(json.dumps(r,indent=2)+'\n')
  try:run()
  except(AssertionError,ValueError,KeyError,IndexError,UnicodeError):passed.append(label)
  else:raise AssertionError('accepted '+label)
 finally:f.write_bytes(old)
def byte(r,address):
 changed=False
 for span in r['ranges']:
  b=bytearray.fromhex(span['bytes']);offset=address-span['address']
  if 0<=offset<len(b):b[offset]^=1;span['bytes']=b.hex();changed=True
 assert changed
for phase,field in(('jobs-before','eip'),('jobs-after','eip'),('jobs-after','esp'),('jobs-after','eax'),('jobs-after','esi'),('mre-5-before','eip'),('mre-5-after','eip'),('mre-5-after','esp'),('mre-5-after','ebx'),('mre-5-after','eax'),('mre-6-before','eip'),('mre-6-after','eip'),('mre-6-after','esp'),('mre-6-after','edi'),('mre-6-after','eax'),('driver-exit','eip')):
 negative(phase+' '+field,'native-'+phase+'-gdb.json',lambda r,n=field:r['general'].__setitem__(n,r['general'][n]^1))
negative('actual jobs formal','native-jobs-before-gdb.json',lambda r:byte(r,r['arguments']['stack']+4))
negative('actual jobs returnword','native-jobs-before-gdb.json',lambda r:byte(r,r['arguments']['stack']))
negative('actual Close commit','native-mre-6-before-gdb.json',lambda r:byte(r,r['arguments']['stack']+8))
negative('actual Close scan','native-mre-6-before-gdb.json',lambda r:byte(r,r['symbols']['first_dependency_scan']))
negative('wrong closed fd handle','native-mre-6-after-gdb.json',lambda r:byte(r,r['data_addresses']['recon_osfhnd']+12))
negative('closed fd wrongly open','native-mre-6-after-gdb.json',lambda r:byte(r,r['data_addresses']['recon_osfile']+3))
negative('release live fd wrongly closed','native-mre-5-after-gdb.json',lambda r:byte(r,r['data_addresses']['recon_osfile']+3))
negative('Close actual error','native-mre-6-after-gdb.json',lambda r:byte(r,r['symbols']['last_error']))
negative('Close fullFP','native-mre-6-after-gdb.json',lambda r:r['x87'].__setitem__('st7','01'+r['x87']['st7'][2:]))
negative('cached replay as fresh','mre-native.json',lambda r:r.__setitem__('cached',True))
negative('missing Close request','mre-native.json',lambda r:r['requests'].pop(11))
negative('released-role extra call','mre-native.json',lambda r:r['requests'].append(copy.deepcopy(r['requests'][-1])))
negative('exit actual call count','native-driver-exit-gdb.json',lambda r:byte(r,r['symbols']['mre_call_count']))
negative('exit stdout fd2 alias diverges','streams-driver-exit.json',lambda r:r['crt'][2].__setitem__('inode',r['crt'][2]['inode']^1))
negative('exit Winstd fd2 alias diverges','streams-driver-exit.json',lambda r:r['win32'][2].__setitem__('handle',r['win32'][2]['handle']^1))
negative('final IDB bytes','case.idb',lambda b:bytes([b[0]^1])+b[1:],raw=True)
def full_status(b):
 x=bytearray(b);struct.pack_into('<I',x,16,256);return bytes(x)
negative('wrong fullDWORD same low8','terminal-status.bin',full_status,raw=True)
run();assert frozen_sources=={n:sha(project/'scripts'/n)for n in SOURCES},'test source changed during campaign'
receipt={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Only new Jobs/lifecycle/released-fd/stdalias/normal-outcome guards, raw restored sequentially; unchanged priorcallback/API1-4/public-state guards retained.','original_capture_sha256':sha(a.original/'capture.json'),'native_capture_sha256':sha(a.native/'capture.json'),'sources':frozen_sources};a.receipt.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
