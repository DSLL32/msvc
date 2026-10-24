#!/usr/bin/env python3
"""Sequential native raw-state controls; restore every byte before returning."""
import argparse,hashlib,json,time
from pathlib import Path
from mre_state import verify
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True);a=p.parse_args();start=time.perf_counter();passed=[]
verify(a.original,a.native,selector_candidate=True)
base={f.name:f.read_bytes()for f in a.native.glob('native-*-gdb.json')}
def reject(label,name,mutate):
 f=a.native/name;old=f.read_bytes();j=json.loads(old)
 try:
  mutate(j);f.write_text(json.dumps(j,indent=2)+'\n')
  try:verify(a.original,a.native,selector_candidate=True)
  except(AssertionError,ValueError,KeyError,IndexError):passed.append(label)
  else:raise AssertionError('accepted '+label)
 finally:f.write_bytes(old)
def field(parent,name,delta=1):return lambda j:j[parent].__setitem__(name,j[parent][name]^delta)
def memory(j,address,offset=0):
 address+=offset;found=False
 for row in j['ranges']:
  b=bytearray.fromhex(row['bytes'])
  if row['address']<=address<row['address']+len(b):b[address-row['address']]^=1;row['bytes']=b.hex();found=True
 assert found
before='native-lower-before-gdb.json';after='native-lower-after-gdb.json';front='native-mre-frontier-gdb.json'
for label,name,change in[
 ('actual lower entry EIP',before,field('general','eip')),
 ('actual lower return EIP',after,field('general','eip')),
 ('lower entry ESP',before,field('general','esp')),
 ('lower cdecl return ESP',after,field('general','esp')),
 ('lower actual result alias',after,field('general','eax')),
 *[('lower nonvolatile '+n,after,field('general',n))for n in('ebx','esi','edi','ebp')],
 ('lower actual input bytes',before,lambda j:memory(j,j['arguments']['values'][0])),
 ('lower actual output bytes',after,lambda j:memory(j,j['arguments']['values'][0])),
 ('lower actual caller return',before,lambda j:memory(j,j['arguments']['stack'])),
 ('lower actual formal slot',before,lambda j:memory(j,j['arguments']['stack'],4)),
 ('lower native state formal',before,lambda j:memory(j,j['arguments']['state_slot'])),
 ('lower actual errno',after,lambda j:memory(j,j['symbols']['errno'])),
 ('lower actual DOS error',after,lambda j:memory(j,j['symbols']['recon_doserrno'])),
 ('lower actual LastError',after,lambda j:memory(j,j['symbols']['last_error'])),
 ('lower actual FP status',after,lambda j:j['x87'].__setitem__('fstat','01000000')),
 ('lower actual locale',after,lambda j:memory(j,j['symbols']['recon_mb_codepage'])),
 ('lower actual FILE word',after,lambda j:memory(j,j['data_addresses']['recon_iob'])),
 ('lower actual fd flags',after,lambda j:memory(j,j['data_addresses']['recon_osfile'],3)),
 ('lower actual fd position',after,lambda j:j['descriptor3'].__setitem__('position',1025)),
 ('missing mandatory lower-before',before,lambda j:j.__setitem__('phase','not-lower-before')),
 ('missing mandatory lower-after',after,lambda j:j.__setitem__('phase','not-lower-after')),
 ('authentic comparator INT3 PC',front,field('general','eip')),
 ('actual comparator left slot',front,field('arguments','left_slot')),
 ('actual comparator right slot',front,field('arguments','right_slot')),
 ('actual qsort saved caller',front,lambda j:memory(j,j['arguments']['qsort_ebp'])),
 ('actual indirect comparator return',front,lambda j:memory(j,j['general']['ebp'],4)),
 ('actual qsort callback pointer',front,lambda j:memory(j,j['arguments']['compare_slot'])),
 ('actual signature saved parent',front,lambda j:memory(j,j['arguments']['signature_ebp'])),
 ('actual signature saved return',front,lambda j:memory(j,j['arguments']['signature_ebp'],4)),
 ('native formal must remain actual Driver',front,lambda j:memory(j,j['arguments']['slot'])),
 ('actual signature item count',front,lambda j:memory(j,j['arguments']['count_slot'])),
 ('actual signature item string',front,lambda j:memory(j,int.from_bytes(next(bytes.fromhex(r['bytes'])for r in j['ranges']if r['address']==j['arguments']['items']),'little')&0xffffffff)),
 ('actual pending output',front,lambda j:memory(j,j['arguments']['pending']))]:reject(label,name,change)
for name,old in base.items():assert(a.native/name).read_bytes()==old,'unrestored '+name
verification=verify(a.original,a.native,selector_candidate=True)
r={'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Direct independent actual native-state verifier; semantic fields mutated, not only transport hashes. Every native raw byte restored; no original execution.','original_sha256':sha(a.original/'capture.json'),'native_sha256':sha(a.native/'capture.json'),'sources':{n:sha(Path(__file__).parent/n)for n in('check-native-selector.py','selector_native_state.py','mre_state.py','mre_elf.py','mre_native.py')},'verification':verification};a.receipt.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
