#!/usr/bin/env python3
"""Pre-use compiled closed-file guards and real filesystem/ack controls only."""
import ctypes,copy,hashlib,json,os,shutil,struct,subprocess,tempfile,time
from pathlib import Path
from original import mre_response_mode,mre_copy_phase
from accept import reconstruction_capture,trial_config
from mre_file import request,copy_stopped_file,publish_ack,allowed_phase
P=Path(__file__).resolve().parents[1];start=time.perf_counter();passed=[]
base=json.loads((P/'functions.json').read_bytes());trial=trial_config(base,'_jobs_from_deferred')
assert mre_response_mode(trial,False,'mre-object',(False,)*7,True)=='CL_MRE_JOBS_RESPONSE_V1\n'
for label,config,baseline,case,other in[
 ('baseline',trial,True,'mre-object',(False,)*7),('wrongcase',trial,False,'mre-prefix',(False,)*7),
 ('laterappunmasked',{**trial,'accepted':trial['accepted']+['_deferred_from_jobs']},False,'mre-object',(False,)*7),
 *[('mixed'+str(i),trial,False,'mre-object',tuple(i==j for j in range(7)))for i in range(7)]]:
 try:mre_response_mode(config,baseline,case,other,True)
 except AssertionError:passed.append(label)
 else:raise AssertionError('bad mode accepted')
for i in range(6):assert mre_copy_phase(('MRE_FILE mre-%d-after\n'%(i+1)).encode(),i,True,False,True)=='mre-%d-after'%(i+1)
for args in((b'MRE_FILE mre-7-after\n',6,True,False,True),(b'MRE_FILE mre-5-after\n',4,True,True,False),(b'MRE_FILE mre-6-after\n',4,True,False,True),(b'MRE_FILE mre-5-after\r\r\n',4,True,False,True),(b'MRE_FILE mre-5-after\n',4,True,True,True)):
 try:mre_copy_phase(*args)
 except AssertionError:passed.append('strict copy sequence '+repr(args))
 else:raise AssertionError('bad copy accepted')
marker=b'CL_MRE_JOBS_RESPONSE_V1\n'
c={'case':'mre-object','baseline':False,'mre_response':True,'helper_status':0,'controller_error':None,'command':['helper','1','0'],'files':{'mre-observer.txt':{'bytes':len(marker),'sha256':hashlib.sha256(marker).hexdigest()}}}
try:reconstruction_capture(c)
except AssertionError:passed.append('new lifecycle acceptance remains denied')
else:raise AssertionError('unimplemented lifecycle admitted')
closed=(P/'scripts/mre-closed-file-observer.inc').read_text();primitive=closed[closed.index('static int mre_closed_state('):closed.index('static void mre_closed_file_copy(')]
jobs=(P/'scripts/mre-jobs-observer.inc').read_text();mask=jobs[jobs.index('static int mre_jobs_mask('):jobs.index('static void mre_jobs_install(')]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='jobs-controls-')as tmp:
 d=Path(tmp);source='#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD;typedef uint8_t BYTE;\ntypedef struct {DWORD attributes,times[6],dwVolumeSerialNumber,sizeHigh,sizeLow,links,nFileIndexHigh,nFileIndexLow;} BY_HANDLE_FILE_INFORMATION;\nstatic int mre_file_have_live=1;static DWORD mre_file_last_slot=0x5000;\n'+primitive.replace('static int','int')+mask.replace('static int','int')+'\nvoid test_cache(unsigned present,unsigned slot){mre_file_have_live=present;mre_file_last_slot=slot;}\n'
 (d/'test.c').write_text(source);subprocess.run(['cc','-shared','-fPIC','-Werror','-Wall','-Wextra',str(d/'test.c'),'-o',str(d/'test.so')],check=True,capture_output=True)
 lib=ctypes.CDLL(str(d/'test.so'));assert lib.mre_closed_state(1,b'mre-6-after',0x5000,0xffffffff,0)==1
 for args in((0,b'mre-6-after',0x5000,0xffffffff,0),(1,b'mre-4-after',0x5000,0xffffffff,0),(1,b'mre-6-after',0x5004,0xffffffff,0),(1,b'mre-6-after',0x5000,44,0),(1,b'mre-6-after',0x5000,0xffffffff,1)):
  assert lib.mre_closed_state(*args)==0;passed.append('compiled closed guard '+repr(args))
 lib.test_cache(0,0x5000);assert lib.mre_closed_state(1,b'mre-6-after',0x5000,0xffffffff,0)==0;passed.append('compiled missing prior live proof')
 lib.test_cache(1,0x5004);assert lib.mre_closed_state(1,b'mre-6-after',0x5000,0xffffffff,0)==0;passed.append('compiled changed prior live slot')
 lib.test_cache(1,0x5000)
 assert lib.mre_jobs_mask(0x55,0xff,0x8b,0x8b)==1
 for v in((0xcc,0xff,0x8b,0x8b),(0x55,0xcc,0x8b,0x8b),(0x55,0xff,0xcc,0x8b),(0x55,0xff,0x8b,0xcc)):
  assert lib.mre_jobs_mask(*v)==0;passed.append('compiled masked caller '+repr(v))
 Array=ctypes.c_uint32*13;a=Array(0x20,*([0]*6),5,0,64,1,0,17);b=Array(*list(a));assert lib.mre_same_file_id(ctypes.byref(a),ctypes.byref(b))==1
 for offset in(7,11,12):
  b=Array(*list(a));b[offset]^=1;assert lib.mre_same_file_id(ctypes.byref(a),ctypes.byref(b))==0;passed.append('compiled actualFileId word'+str(offset))
 # Changes across the real API may affect size/timestamps; the observer then
 # requires complete equality of CURRENT information before/after the copy.
 b=Array(*list(a));b[9]=32;b[3]=123;assert lib.mre_same_file_id(ctypes.byref(a),ctypes.byref(b))==1
 observer=d/'observer';observer.mkdir();(d/'mre-observer.txt').write_bytes(marker);(d/'case.idb').write_bytes(bytes(range(32)))
 phase='mre-6-after';prefix=observer/('mre-files-idb-'+phase+'-');path=lambda n:Path(str(prefix)+n)
 current=struct.pack('<13I',*b);previous=struct.pack('<13I',*a)
 prev=observer/'mre-files-idb-mre-4-after-'
 for suffix,value in(('request.bin',struct.pack('<8I',1,1,2,0x5000,44,1024,0,999)),('descriptor.bin',previous),('context-after.bin',b'proof'),('host.json',b'{}'),('path-reopened.bin',previous)):Path(str(prev)+suffix).write_bytes(value)
 initial={'request.bin':struct.pack('<8I',2,3,2,0x5000,0xffffffff,0,0,999),'path.bin':current,'last-live.bin':previous,'lowio.bin':struct.pack('<I',0xffffffff)+bytes(32),'cwd.bin':('S:\\vc5.10\\cl\\'+d.name+'\0').encode('utf-16le'),'dos-path.bin':('S:\\vc5.10\\cl\\'+d.name+'\\case.idb\0').encode(),'path-open.bin':struct.pack('<6I',42,0,1,0,0,7)}
 for n,value in initial.items():path(n).write_bytes(value)
 assert request(d,phase)['closed']is True
 def mutate_word(offset,value):
  def change(raw):
   b=bytearray(raw);struct.pack_into('<I',b,offset,value);return bytes(b)
  return change
 def reject(label,name,change):
  f=path(name);old=f.read_bytes()
  try:
   f.write_bytes(change(old))
   try:request(d,phase)
   except(AssertionError,ValueError,UnicodeError,struct.error):passed.append(label)
   else:raise AssertionError('accepted '+label)
  finally:f.write_bytes(old)
 for offset,value,label in((0,3,'wrong schema'),(4,0,'missing event'),(8,0,'missing thread'),(12,0,'missing slot'),(12,0x5004,'changed lowio slot'),(16,44,'still live handle'),(20,1,'invented cursor'),(24,1,'invented high cursor'),(28,0,'missing pid')):reject(label,'request.bin',mutate_word(offset,value))
 reject('open lowio in closed branch','lowio.bin',lambda b:b[:4]+b'\x01'+b[5:])
 for offset in(28,44,48):reject('wrong path Windows FileId'+str(offset),'path.bin',mutate_word(offset,777))
 reject('last live record unbound','last-live.bin',mutate_word(4,777))
 reject('metadata access requested read','path-open.bin',mutate_word(16,0x80000000))
 reject('directory output','path.bin',mutate_word(0,0x10));reject('reparse output','path.bin',mutate_word(0,0x400))
 reject('wrong actual DOS path','dos-path.bin',lambda b:b.replace(b'case.idb',b'fake.idb'))
 for wrong in(b'CL_MRE_SORT_RESPONSE_V1\n',b'CL_MRE_JOBS_RESPONSE_V1',b'CL_MRE_JOBS_RESPONSE_V1\r\n'):
  (d/'mre-observer.txt').write_bytes(wrong);assert not allowed_phase(d,phase);passed.append('closed wrongfamily '+repr(wrong))
 (d/'mre-observer.txt').write_bytes(marker)
 # Real NOATIME/NOFOLLOW copy and atomic bound acknowledgment on this synthetic
 # Windows packet. This tests transport, never supplies original target proof.
 before=os.stat(d/'case.idb');receipt=copy_stopped_file(d,phase);after=os.stat(d/'case.idb')
 assert before.st_atime_ns==after.st_atime_ns and path('contents.bin').read_bytes()==bytes(range(32))
 publish_ack(d,phase,receipt);assert path('ack.bin').read_bytes()==initial['request.bin']+phase.encode()+b'\0'
 try:publish_ack(d,phase,receipt)
 except(AssertionError,FileExistsError):passed.append('duplicate actual ack')
 else:raise AssertionError('ack overwritten')
 for snippet in('memcmp(&before_context,&after_context','memcmp(lowio,after_lowio,36)','memcmp(&info,&after,sizeof(info))','GetFileInformationByHandle(reopened','mre_same_file_id(&info,&mre_file_last_live)'):assert snippet in closed
print(json.dumps({'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Compiled actual mask/closedidentity guards and actual Linux NOATIME/ack filesystem, synthetic Windows packet only; no original/case/provider acceptance','reuse':'Unchanged open-file premise and atomic32B request+phase protocol retain prior59/103 controls; new schema2/phase5-6 branches tested here.'},indent=2))
