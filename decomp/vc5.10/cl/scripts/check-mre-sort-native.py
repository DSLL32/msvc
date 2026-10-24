#!/usr/bin/env python3
"""Affected comparator/API4 addressed-state controls; restore every raw record."""
import argparse,copy,hashlib,json,time
from pathlib import Path
from mre_state import verify
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True);a=p.parse_args();passed=[];start=time.perf_counter()
run=lambda:verify(a.original,a.native,sort_candidate=True)
run()
def negative(label,name,mutate):
 f=a.native/name;old=f.read_bytes()
 try:
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
for phase,field in(('compare-before','eip'),('compare-after','eip'),('compare-after','esp'),('compare-after','ebx'),('mre-4-before','eip'),('mre-4-after','eip'),('mre-4-after','esp'),('mre-4-after','esi'),('mre-frontier','eip')):
 negative('actual '+phase+' '+field,'native-'+phase+'-gdb.json',lambda r,n=field:r['general'].__setitem__(n,r['general'][n]^1))
for phase,value in(('compare-after',19),('mre-4-after',0)):
 negative('actual '+phase+' result','native-'+phase+'-gdb.json',lambda r,v=value:r['general'].__setitem__('eax',v))
negative('callback input slot alias','native-compare-before-gdb.json',lambda r:r['arguments'].__setitem__('right',r['arguments']['left']))
negative('callback owned string bytes','native-compare-after-gdb.json',lambda r:byte(r,r['arguments']['items']))
negative('callback saved qsort return','native-compare-before-gdb.json',lambda r:byte(r,r['arguments']['stack']))
negative('callback full FP data','native-compare-after-gdb.json',lambda r:r['x87'].__setitem__('st7','01'+r['x87']['st7'][2:]))
negative('actual fourth pending input','native-mre-4-before-gdb.json',lambda r:byte(r,r['arguments']['values'][1]))
negative('actual fourth pending output','native-mre-4-after-gdb.json',lambda r:byte(r,r['arguments']['values'][1]+16))
for name in('errno','last_error','mre_call_count'):
 negative('actual fourth '+name,'native-mre-4-after-gdb.json',lambda r,n=name:byte(r,r['symbols'][n]))
negative('actual fourth fd position','native-mre-4-after-gdb.json',lambda r:r['descriptor3'].__setitem__('position',1025))
negative('actual fourth lowio flags','native-mre-4-after-gdb.json',lambda r:byte(r,r['data_addresses']['recon_osfile']+3))
negative('actual frontier formal','native-mre-frontier-gdb.json',lambda r:byte(r,r['arguments']['slot']))
negative('actual frontier saved caller','native-mre-frontier-gdb.json',lambda r:byte(r,r['general']['ebp']+4))
negative('missing fourth request','mre-native.json',lambda r:r['requests'].pop(7))
negative('extra fourth request','mre-native.json',lambda r:r['requests'].append(copy.deepcopy(r['requests'][-1])))
negative('cross PID fourth request','mre-native.json',lambda r:r['requests'][7].__setitem__('pid',r['pid']+1))
run();sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();project=Path(__file__).resolve().parents[1]
receipt={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Only affected callback/API4/nextfrontier controls; raw restored sequentially. Unchanged API1-3/ownership/file transport and current35 protocol controls retained separately.','original_capture_sha256':sha(a.original/'capture.json'),'native_capture_sha256':sha(a.native/'capture.json'),'sources':{n:sha(project/'scripts'/n)for n in('check-mre-sort-native.py','mre_sort_packet.py','mre_native.py','mre_state.py','sort_native_state.py','mre_elf.py')}}
a.receipt.write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
