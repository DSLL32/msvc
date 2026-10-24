#!/usr/bin/env python3
"""Pure Python/compiled C pre-use controls for the signature candidate observer."""
import ctypes,hashlib,json,subprocess,tempfile,time
from pathlib import Path
from original import mre_response_mode
from accept import reconstruction_capture

PROJECT=Path(__file__).resolve().parents[1];started=time.perf_counter();passed=[]
base=json.loads((PROJECT/'functions.json').read_bytes());trial={**base,'accepted':base['accepted']+['_option_signature']}
assert mre_response_mode(base,False,'mre-object',(False,)*7,True)=='CL_MRE_PREFIX_RESPONSE_V2\n'
assert mre_response_mode(trial,False,'mre-object',(False,)*7,True)=='CL_MRE_SIGNATURE_RESPONSE_V1\n'
for label,config,baseline,case,others in[
 ('baseline',trial,True,'mre-object',(False,)*7),('wrongcase',trial,False,'mre-prefix',(False,)*7),
 ('accepted selector',{**trial,'accepted':trial['accepted']+['_option_matching_selector']},False,'mre-object',(False,)*7),
 *[('mixed family '+str(i),trial,False,'mre-object',tuple(i==j for j in range(7)))for i in range(7)],
 *[('unaccepted caller '+name,{**trial,'accepted':[v for v in trial['accepted']if v!=name]},False,'mre-object',(False,)*7)for name in('_prepare_jobs','_execute_pass')]]:
 try:mre_response_mode(config,baseline,case,others,True)
 except AssertionError:passed.append(label)
 else:raise AssertionError('accepted '+label)
source=(PROJECT/'scripts/mre-observer.inc').read_text()
guards=source[source.index('static int mre_observation_mode('):source.index('static void mre_observation_install(')]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='signature-mode-')as temporary:
 directory=Path(temporary);c=directory/'guards.c';so=directory/'guards.so'
 c.write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD;typedef unsigned char BYTE;\n'+guards.replace('static int','int'))
 subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(c),'-o',str(so)],check=True,capture_output=True)
 lib=ctypes.CDLL(str(so))
 assert lib.mre_observation_frontier_mode(0,0xcc,0xcc)==1
 assert lib.mre_observation_frontier_mode(1,0x55,0xcc)==1
 for args in((1,0xcc,0xcc),(1,0x55,0x55),(1,0x55,0),(1,0,0xcc),(0,0x55,0xcc),(2,0x55,0xcc),(-1,0x55,0xcc)):
  assert lib.mre_observation_frontier_mode(*args)==0;passed.append('compiled wrong mask ownership '+repr(args))
 for marker in(b'CL_MRE_PREFIX_RESPONSE_V2\n',b'CL_MRE_SIGNATURE_RESPONSE_V1\n'):assert lib.mre_observation_response_marker(marker)==1
 for marker in(b'CL_MRE_SIGNATURE_RESPONSE_V1',b'CL_MRE_SIGNATURE_RESPONSE_V1\r\n',b'CL_MRE_SIGNATURE_RESPONSE_V1\nextra',b'CL_MRE_SIGNATURE_RESPONSE_V2\n'):
  assert lib.mre_observation_response_marker(marker)==0;passed.append('compiled exact marker '+repr(marker))
 for masked,other in((0,0),(0,1),(1,1),(2,0)):
  assert lib.mre_observation_response_mode(masked,other)==0;passed.append('compiled wrong response family '+repr((masked,other)))
for baseline in(False,True):
 try:reconstruction_capture({'baseline':baseline,'mre_response':True})
 except AssertionError:passed.append('acceptance still held until concrete fresh provider '+str(baseline))
 else:raise AssertionError('accepted unfinished signature mode')
assert 'memory(target_base+0x10f7,&selector,1)'in source
assert 'address!=target_base+0x10f7 || arguments[0]!=target_base+0x3989'in source
assert 'frame[0]!=mre_analysis_ebp || frame[1]!=target_base+0x35f2 || frame[2]!=state'in source
assert 'memcmp(selectors,"Mm",3)'in source
assert 'memory(target_base+0x9010,&include_flag,4)'in source
assert 'if (!mre_response_observer && address==target_base+0x393a)'in source
assert 'mre-signature-entry'in source # Separate old unmasked analysis path preserved.
report={'passed':True,'controls':passed,'count':len(passed),'seconds':time.perf_counter()-started,
 'scope':'No original execution. Fresh masked signature body may reach only genuine unaccepted selector; original API/file observers unchanged.',
 'sources':{n:hashlib.sha256((PROJECT/'scripts'/n).read_bytes()).hexdigest()for n in('original.py','mre-observer.inc','mre-file-observer.inc','mre_file.py','accept.py')}}
print(json.dumps(report,indent=2))
