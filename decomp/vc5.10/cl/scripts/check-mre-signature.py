#!/usr/bin/env python3
"""Affected signature candidate input/frontier controls; original runs are reused."""
import argparse,copy,hashlib,json,shutil,struct,tempfile,time
from pathlib import Path
from mre_signature_packet import decode
from mre_state import verify

p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True)
a=p.parse_args();passed=[];started=time.perf_counter();decode(a.original);verify(a.original,a.native,signature_candidate=True)
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='signature-original-controls-')as temporary:
 directory=Path(temporary)/'original';shutil.copytree(a.original,directory)
 original_capture=(directory/'capture.json').read_bytes()
 def original_negative(name,changes):
  backups={filename:(directory/filename).read_bytes()for filename in changes}
  try:
   capture=json.loads(original_capture)
   for filename,change in changes.items():
    value=change(backups[filename]);(directory/filename).write_bytes(value)
    capture['files'][filename]={'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
   (directory/'capture.json').write_text(json.dumps(capture,indent=2)+'\n')
   try:decode(directory)
   except(AssertionError,ValueError,KeyError):passed.append(name)
   else:raise AssertionError('accepted '+name)
  finally:
   for filename,value in backups.items():(directory/filename).write_bytes(value)
   (directory/'capture.json').write_bytes(original_capture)
 def flip(value,offset=0):
  changed=bytearray(value);changed[offset]^=1;return bytes(changed)
 for filename,label in[
  ('observer/mre-frontier-use-standard-include.bin','live include flag'),('observer/mre-frontier-include-path.bin','live include bytes'),
  ('observer/mre-frontier-selectors.bin','actual Mm selector string'),('observer/mre-frontier-formal-arguments.bin','actual selector formal'),
  ('observer/mre-frontier-signature-frame.bin','saved signature caller EBP'),
  ('observer/mre-2-after-driver-out.bin','real external driver role'),('observer/mre-1-after-error-out.bin','real external Open error out'),
  ('observer/mre-files-idb-mre-1-after-contents.bin','actual intermediate IDB bytes')]:
  original_negative('original '+label,{filename:flip})
 original_negative('original actual caller return',{'observer/mre-frontier-signature-frame.bin':lambda v:flip(v,4)})
 original_negative('original signature state formal',{'observer/mre-frontier-signature-frame.bin':lambda v:flip(v,8)})
 original_negative('old prefix marker cannot bless candidate',{'mre-observer.txt':lambda v:b'CL_MRE_PREFIX_RESPONSE_V2\n'})
 original_negative('missing candidate in raw snapshot',{'config.json':lambda v:(json.dumps({**json.loads(v),'accepted':[x for x in json.loads(v)['accepted']if x!='_option_signature']},indent=2)+'\n').encode()})
 # Even agreeing duplicated formal blobs cannot choose a different option.
 def option(value):
  value=bytearray(value);struct.pack_into('<I',value,0,struct.unpack_from('<I',value)[0]+4);return bytes(value)
 def stack(value):
  value=bytearray(value);struct.pack_into('<I',value,4,struct.unpack_from('<I',value,4)[0]+4);return bytes(value)
 original_negative('coherent wrong first Option role',{'observer/mre-frontier-formal-arguments.bin':option,'observer/mre-frontier-stack.bin':stack})

def negative(name,filename,change):
 path=a.native/filename;raw=path.read_bytes()
 try:
  record=json.loads(raw);change(record);path.write_text(json.dumps(record,indent=2)+'\n')
  try:verify(a.original,a.native,signature_candidate=True)
  except(AssertionError,ValueError,KeyError):passed.append(name)
  else:raise AssertionError('accepted '+name)
 finally:path.write_bytes(raw)
def byte(record,address,offset=0):
 changed=False
 for span in record['ranges']:
  value=bytearray.fromhex(span['bytes']);index=address+offset-span['address']
  if 0<=index<len(value):value[index]^=1;span['bytes']=value.hex();changed=True
 assert changed
file='native-mre-frontier-gdb.json'
negative('native exact selector INT3 PC',file,lambda r:r['general'].__setitem__('eip',r['general']['eip']+1))
negative('native selector saved caller EBP',file,lambda r:byte(r,r['general']['ebp']))
negative('native selector saved return',file,lambda r:byte(r,r['general']['ebp'],4))
negative('native signature saved caller EBP',file,lambda r:byte(r,r['arguments']['signature_ebp']))
negative('native signature saved return',file,lambda r:byte(r,r['arguments']['signature_ebp'],4))
for name in('slot','option_slot','selectors_slot'):
 negative('native formal slot '+name,file,lambda r,n=name:r['arguments'].__setitem__(n,r['arguments'][n]+4))
negative('native actual first Option formal',file,lambda r:byte(r,r['arguments']['option_slot']))
negative('native actual selector formal',file,lambda r:byte(r,r['arguments']['selectors_slot']))
negative('native actual selectors content',file,lambda r:byte(r,r['arguments']['selectors']))
negative('native consumed include flag',file,lambda r:byte(r,r['symbols']['use_standard_include']))
def include(record):
 from mre_state import Memory
 m=Memory(record['ranges']);driver=m.word(record['symbols']['driver']);pointer=m.word(driver+36);byte(record,pointer)
negative('native consumed include content',file,include)
negative('native source pending producer',file,lambda r:byte(r,r['arguments']['pending']))
for name in('errno','recon_doserrno','last_error'):
 negative('native live error '+name,file,lambda r,n=name:byte(r,r['symbols'][n]))
for name in('ebx','esi','edi','ebp'):
 negative('native real return nonvolatile '+name,'native-mre-3-after-gdb.json',lambda r,n=name:r['general'].__setitem__(n,r['general'][n]^1))
negative('native mandatory inventory','mre-native.json',lambda r:r['requests'].pop())
negative('native bound ELF','native-execution.json',lambda r:r.__setitem__('native_sha256','0'*64))
verify(a.original,a.native,signature_candidate=True)
report={'passed':True,'count':len(passed),'seconds':time.perf_counter()-started,'controls':passed,
 'scope':'Fresh signature input/real caller+formal/FP-state boundary controls; old raw restored; no original run.',
 'reused':'Initial prefix57 ABI/effect controls and11 ELF controls remain applicable to unchanged C provider7d3f75e9/MRE wrappers e0e6452c and same shared contract. New native actually dumps both saved frames.'}
a.receipt.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
