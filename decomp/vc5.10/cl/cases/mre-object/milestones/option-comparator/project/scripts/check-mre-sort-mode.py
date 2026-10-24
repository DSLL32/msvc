#!/usr/bin/env python3
"""Pre-use guards for the fourth-MRE/first-comparator original-only slice."""
import ctypes,hashlib,json,subprocess,tempfile,time
from pathlib import Path
from original import mre_response_mode,mre_copy_phase
from accept import reconstruction_capture,trial_config
from mre_file import allowed_phase
P=Path(__file__).resolve().parents[1];start=time.perf_counter();passed=[]
base=json.loads((P/'functions.json').read_bytes());trial=trial_config(base,'_compare_option_strings')
assert mre_response_mode(base,False,'mre-object',(False,)*7,True)=='CL_MRE_SELECTOR_RESPONSE_V1\n'
assert mre_response_mode(trial,False,'mre-object',(False,)*7,True)=='CL_MRE_SORT_RESPONSE_V1\n'
for label,config,baseline,case,others in[
 ('baseline',trial,True,'mre-object',(False,)*7),('wrongcase',trial,False,'mre-prefix',(False,)*7),
 ('jobs alreadyaccepted',{**trial,'accepted':trial['accepted']+['_jobs_from_deferred']},False,'mre-object',(False,)*7),
 *[('mixedfamily'+str(i),trial,False,'mre-object',tuple(i==j for j in range(7)))for i in range(7)],
 *[('unacceptedcaller'+n,{**trial,'accepted':[v for v in trial['accepted']if v!=n]},False,'mre-object',(False,)*7)for n in('_prepare_jobs','_execute_pass')]]:
 try:mre_response_mode(config,baseline,case,others,True)
 except AssertionError:passed.append(label)
 else:raise AssertionError('accepted '+label)
s=(P/'scripts/mre-sort-observer.inc').read_text();primitive=s[s.index('static int mre_sort_mask('):s.index('static void mre_sort_install(')]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='sort-mode-')as tmp:
 d=Path(tmp);(d/'test.c').write_text('typedef unsigned char BYTE;\n'+primitive.replace('static int','int'))
 subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(d/'test.c'),'-o',str(d/'test.so')],check=True,capture_output=True)
 lib=ctypes.CDLL(str(d/'test.so'));assert lib.mre_sort_mask(0x55,0xff,0x8b,0xcc)==1
 for args in((0xcc,0xff,0x8b,0xcc),(0x55,0xcc,0x8b,0xcc),(0x55,0xff,0xcc,0xcc),(0x55,0xff,0x8b,0x8b),(0x55,0xff,0x8b,0)):
  assert lib.mre_sort_mask(*args)==0;passed.append('compiled actualmask '+repr(args))
 for marker in(b'CL_MRE_PREFIX_RESPONSE_V2\n',b'CL_MRE_SIGNATURE_RESPONSE_V1\n',b'CL_MRE_SELECTOR_RESPONSE_V1\n',b'CL_MRE_SORT_RESPONSE_V1',b'CL_MRE_SORT_RESPONSE_V1\r\n',b'CL_MRE_SORT_RESPONSE_V1\nextra'):
  (d/'mre-observer.txt').write_bytes(marker);assert not allowed_phase(d,'mre-4-after');passed.append('actual fourthcopy rejects '+repr(marker))
 (d/'mre-observer.txt').write_bytes(b'CL_MRE_SORT_RESPONSE_V1\n');assert allowed_phase(d,'mre-4-after')
 for phase in('mre-0-after','mre-5-after','mre-4-before','../mre-4-after'):
  assert not allowed_phase(d,phase);passed.append('unknown copyphase '+phase)
assert mre_copy_phase(b'MRE_FILE mre-4-after\r\n',3,True,True)=='mre-4-after'
for args in((b'MRE_FILE mre-4-after\n',3,True,False),(b'MRE_FILE mre-4-after\n',4,True,True),(b'MRE_FILE mre-3-after\n',3,True,True),(b'MRE_FILE mre-4-after\n',3,False,True),(b'MRE_FILE mre-4-after\r\r\n',3,True,True)):
 try:mre_copy_phase(*args)
 except AssertionError:passed.append('exact sequence/copyfamily '+repr(args))
 else:raise AssertionError('accepted badcopy '+repr(args))
marker=b'CL_MRE_SORT_RESPONSE_V1\n';capture={'case':'mre-object','baseline':False,'mre_response':True,'helper_status':0,'controller_error':None,'command':['helper','1','0'],'files':{'mre-observer.txt':{'bytes':len(marker),'sha256':hashlib.sha256(marker).hexdigest()}}}
reconstruction_capture(capture) # Mode admission only; validate() still requires full fresh state/audit.
for marker in(b'CL_MRE_SORT_RESPONSE_V1',b'CL_MRE_SORT_RESPONSE_V1\r\n',b'CL_MRE_SORT_RESPONSE_V1\nextra'):
 bad={**capture,'files':{'mre-observer.txt':{'bytes':len(marker),'sha256':hashlib.sha256(marker).hexdigest()}}}
 try:reconstruction_capture(bad)
 except AssertionError:passed.append('admission rejects altered exact marker '+repr(marker))
 else:raise AssertionError('accepted altered marker')
for snippet in('context->Esp!=mre_compare_stack+4','mre_sort_arguments[1]!=work','actual!=exported(mre_provider_base,"_MREDrvFFilesOutOfDate@8")','mre_file_copy(phase','mre_sort_calls!=1','mre_compare_calls!=1'):
 assert snippet in s,snippet
print(json.dumps({'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'One masked comparator candidate; first actual callback return + one fourth MRE call only. Later unknowns guarded; original/private result not guessed; no native API4 success.','reuse':'Unchanged59 file-identity and103 atomicack controls; changed fourthphase/mode paths covered here.'},indent=2))
