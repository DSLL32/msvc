#!/usr/bin/env python3
"""Focused new-field schema tests; synthetic data gives no execution credit."""
import ctypes,json,subprocess,tempfile,time
from pathlib import Path
from mre_jobs_controls import first_scan,close_commit
start=time.perf_counter();passed=[]
base={'mre-observer.txt':b'CL_MRE_JOBS_RESPONSE_V1\n','observer/mre-6-before-first-dependency-scan.bin':b'\1\0\0\0'}
assert first_scan(base.__getitem__,'mre-6-before')==1;close_commit(base.__getitem__,'mre-6-before',[0x5000,1])
for label,change,args in[
 ('missing field',lambda d:d.pop('observer/mre-6-before-first-dependency-scan.bin'),[0x5000,1]),
 ('wrong width',lambda d:d.__setitem__('observer/mre-6-before-first-dependency-scan.bin',b'\1'),[0x5000,1]),
 ('unknown value',lambda d:d.__setitem__('observer/mre-6-before-first-dependency-scan.bin',b'\2\0\0\0'),[0x5000,1]),
 ('different scalar',lambda d:d.__setitem__('observer/mre-6-before-first-dependency-scan.bin',b'\0'*4),[0x5000,1]),
 ('wrong commit',lambda d:None,[0x5000,0]),('bool not DWORD',lambda d:None,[0x5000,True]),
 ('wrong family',lambda d:d.__setitem__('mre-observer.txt',b'CL_MRE_SORT_RESPONSE_V1\n'),[0x5000,1]),
 ('wrong formal count',lambda d:None,[0x5000])]:
 d=dict(base);change(d)
 try:close_commit(d.__getitem__,'mre-6-before',args)
 except(AssertionError,KeyError):passed.append(label)
 else:raise AssertionError('accepted '+label)
project=Path(__file__).resolve().parents[1]
source=(project/'scripts/mre-observer.inc').read_text()
body=source[source.index('static void mre_jobs_control_snapshot('):source.index('static int mre_observation_mode(')]
with tempfile.TemporaryDirectory(dir='/root/.ai-tmp/msvc2/vc5.10/cl',prefix='jobs-scalar-')as tmp:
 d=Path(tmp)
 unit='#include <stdint.h>\n#include <string.h>\n#include <assert.h>\ntypedef uint32_t DWORD;static int mre_jobs_observer;static DWORD target_base=0x400000,sample,saved;static unsigned reads,saves;\nvoid memory(DWORD address,void*buffer,unsigned size){assert(address==0x409018 && size==4);memcpy(buffer,&sample,4);++reads;}\nvoid dependency_save(const char*phase,const char*name,const void*buffer,unsigned size){assert(!strcmp(phase,"mre-6-before")&&!strcmp(name,"first-dependency-scan.bin")&&size==4);memcpy(&saved,buffer,4);++saves;}\n'+body+'\nunsigned probe(int enabled,DWORD value){mre_jobs_observer=enabled;sample=value;reads=saves=0;saved=0;mre_jobs_control_snapshot("mre-6-before");assert(reads==saves);return reads;}\nDWORD value(void){return saved;}\n'
 (d/'probe.c').write_text(unit);subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(d/'probe.c'),'-o',str(d/'probe.so')],check=True,capture_output=True)
 lib=ctypes.CDLL(str(d/'probe.so'));lib.value.restype=ctypes.c_uint32
 assert lib.probe(0,1)==0;passed.append('compiled oldfamily no read/no record')
 for value in(0,1,0xdeadbeef):
  assert lib.probe(1,value)==1 and lib.value()==value;passed.append('compiled exact actual DWORD '+hex(value))
print(json.dumps({'passed':True,'count':len(passed),'controls':passed,'seconds':time.perf_counter()-start,'scope':'Synthetic new scalar/commit schema only; actual original response still absent'},indent=2))
