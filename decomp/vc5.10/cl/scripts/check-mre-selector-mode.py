#!/usr/bin/env python3
"""Bounded pre-use controls; no original execution or lowercase success."""
import copy,ctypes,hashlib,json,subprocess,tempfile,time
from pathlib import Path
from original import mre_response_mode
from accept import reconstruction_capture,trial_config
from binary import PE
P=Path(__file__).resolve().parents[1];R=P.parents[2];start=time.perf_counter();passed=[]
base=json.loads((P/'functions.json').read_text());trial=trial_config(base,'_option_matching_selector')
assert mre_response_mode(base,False,'mre-object',(False,)*7,True)=='CL_MRE_SIGNATURE_RESPONSE_V1\n'
assert mre_response_mode(trial,False,'mre-object',(False,)*7,True)=='CL_MRE_SELECTOR_RESPONSE_V1\n'
for label,config,baseline,case,others in[
 ('baseline',trial,True,'mre-object',(False,)*7),('wrongcase',trial,False,'mre-prefix',(False,)*7),
 ('comparator accepted',{**trial,'accepted':trial['accepted']+['_compare_option_strings']},False,'mre-object',(False,)*7),
 *[('mixed family '+str(i),trial,False,'mre-object',tuple(i==j for j in range(7)))for i in range(7)],
 *[('unaccepted caller '+name,{**trial,'accepted':[v for v in trial['accepted']if v!=name]},False,'mre-object',(False,)*7)for name in('_prepare_jobs','_execute_pass')]]:
 try:mre_response_mode(config,baseline,case,others,True)
 except AssertionError:passed.append(label)
 else:raise AssertionError('accepted '+label)
s=(P/'scripts/mre-observer.inc').read_text();guards=s[s.index('static int mre_observation_mode('):s.index('static void mre_observation_install(')]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='selector-mode-')as td:
 d=Path(td);(d/'guards.c').write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD;typedef unsigned char BYTE;\n'+guards.replace('static int','int'))
 subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(d/'guards.c'),'-o',str(d/'guards.so')],check=True,capture_output=True)
 lib=ctypes.CDLL(str(d/'guards.so'));assert lib.mre_selector_frontier_mode(0x55,0xff,0xcc)==1
 for args in((0xcc,0xff,0xcc),(0x55,0xcc,0xcc),(0x55,0xff,0x8b),(0x55,0xff,0),(0,0xff,0xcc)):
  assert lib.mre_selector_frontier_mode(*args)==0;passed.append('compiled real accepted/unaccepted ownership '+repr(args))
 assert lib.mre_observation_response_marker(b'CL_MRE_SELECTOR_RESPONSE_V1\n')==1
 for marker in(b'CL_MRE_SELECTOR_RESPONSE_V1',b'CL_MRE_SELECTOR_RESPONSE_V1\r\n',b'CL_MRE_SELECTOR_RESPONSE_V1\nextra',b'CL_MRE_SELECTOR_RESPONSE_V2\n'):
  assert lib.mre_observation_response_marker(marker)==0;passed.append('compiled exact marker '+repr(marker))
 for masked,other in((0,0),(0,1),(1,1),(2,0)):
  assert lib.mre_observation_response_mode(masked,other)==0;passed.append('compiled wrong response family '+repr((masked,other)))
pe=PE(R/'toolchains/5.10/bin/CL.EXE');assert pe.read(0x403abb,6)==bytes.fromhex('ff156ca24000')and pe.read(0x403ac1,1)==b'\x8b'
marker=b'CL_MRE_SELECTOR_RESPONSE_V1\n';c={'case':'mre-object','baseline':False,'mre_response':True,'files':{'mre-observer.txt':{'bytes':len(marker),'sha256':hashlib.sha256(marker).hexdigest()}},'command':['helper','1','0'],'helper_status':0,'controller_error':None}
reconstruction_capture(c) # Family admission alone is never full raw/state acceptance.
for key,value in(('baseline',True),('case','mre-prefix'),('helper_status',188),('controller_error','failed'),('dependency_response',True)):
 bad={**c,key:value}
 try:reconstruction_capture(bad)
 except AssertionError:passed.append('reject wrong complete-family field '+key)
 else:raise AssertionError('wrong complete-family field accepted '+key)
for marker in(b'CL_MRE_PREFIX_RESPONSE_V2\n',b'CL_MRE_SELECTOR_RESPONSE_V2\n'):
 bad={**c,'files':{'mre-observer.txt':{'bytes':len(marker),'sha256':hashlib.sha256(marker).hexdigest()}}}
 try:reconstruction_capture(bad)
 except AssertionError:passed.append('reject unreviewed marker '+repr(marker))
 else:raise AssertionError('unreviewed marker accepted')
for text in('context->Eax!=mre_lower_pointer','context->Esp!=mre_lower_esp','actual!=exported(crt_base,"_mbslwr")','mre_lower_calls!=1 || mre_lower_pending','mre_lower_length=i+1','memory(mre_lower_pointer,text,mre_lower_length)','count>64','(arguments[1]-array)%4','patch_byte(target_base+0x3abb,0xcc)'):
 assert text in s,text
print(json.dumps({'passed':True,'controls':passed,'count':len(passed),'seconds':time.perf_counter()-start,'scope':'Next required masked selector capture; exact one reached include-lower call/return and genuine comparator frontier only. Full raw/state/ABI checks remain mandatory after family admission.','reuse':'Unchanged initialMRE/filecopy59+103 controls and signature scope controls retain historical receipts; changed mode/ownership guards refreshed here.'},indent=2))
