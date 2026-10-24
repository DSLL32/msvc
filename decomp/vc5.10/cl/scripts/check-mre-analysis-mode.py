#!/usr/bin/env python3
"""Bounded MRE analysis guards only; no target/original execution."""
import ctypes,itertools,json,struct,subprocess,tempfile,time
from pathlib import Path
from original import mre_analysis_mode
from accept import reconstruction_capture
from binary import PE

PROJECT=Path(__file__).resolve().parents[1];passed=[];started=time.perf_counter()
assert mre_analysis_mode(True,'mre-prefix',False,False,False,False,False,False,True)=='CL_MRE_PREFIX_ANALYSIS_V2\n'
for values in itertools.product((False,True),repeat=7):
    if values==(True,False,False,False,False,False,False):continue
    try:mre_analysis_mode(values[0],'mre-prefix',*values[1:],True)
    except AssertionError:passed.append('Python baseline/family '+str(values))
    else:raise AssertionError('accepted mixed analysis mode '+str(values))
try:mre_analysis_mode(True,'minimal-c',False,False,False,False,False,False,True)
except AssertionError:passed.append('wrong case')
else:raise AssertionError('accepted wrong case')
for baseline in(False,True):
    try:reconstruction_capture({'mre_analysis':True,'baseline':baseline})
    except AssertionError:passed.append('analysis never acceptance '+str(baseline))
    else:raise AssertionError('accepted analysis')
source=(PROJECT/'scripts/mre-observer.inc').read_text()
a=source.index('static int mre_observation_mode(');b=source.index('static void mre_observation_install(',a)
with tempfile.TemporaryDirectory(prefix='mre-mode-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as name:
    directory=Path(name);c=directory/'guard.c';library=directory/'guard.so'
    c.write_text('#include <stdint.h>\n#include <string.h>\ntypedef uint32_t DWORD;typedef unsigned char BYTE;\n'+source[a:b].replace('static int','int'))
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(c),'-o',str(library)],check=True,capture_output=True)
    lib=ctypes.CDLL(str(library));assert lib.mre_observation_mode(0,0)==1
    for args in [(1,0),(0,1),(1,1)]:assert lib.mre_observation_mode(*args)==0;passed.append('C mode '+str(args))
    pe=PE(PROJECT.parents[2]/'toolchains/5.10/bin/CL.EXE')
    for site,slot in [(0x3597,0xa300),(0x35c1,0xa314),(0x35ca,0xa304)]:
        raw=pe.read(pe.base+site,6);value=ctypes.create_string_buffer(raw)
        assert lib.mre_observation_call(value,pe.base,slot)==1
        for offset in range(6):
            changed=bytearray(raw);changed[offset]^=1
            assert lib.mre_observation_call(ctypes.create_string_buffer(bytes(changed)),pe.base,slot)==0
            passed.append('C actual call byte %x/%d'%(site,offset))
        assert lib.mre_observation_call(value,pe.base+0x10000,slot)==0;passed.append('wrong relocated base '+hex(site))
assert 'mre_analysis_calls!=(DWORD)point'in source
assert 'mre_analysis_stack[point]+mre_analysis_bytes[point]'in source
assert 'actual!=exported(mre_provider_base,mre_analysis_exports[point])'in source
assert 'TerminateProcess(child,0x7004)'in source
assert 'mre_analysis_calls!=3'in source
assert 'memory(input,bytes,16)'in source and 'memory(input,bytes,20)'not in source
assert pe.read(0x401063,3)==bytes.fromhex('566a10'),'Input allocation is not16 bytes'
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'controls':passed,'scope':'pre-use compiled mode/call primitives and analysis rejection only'}))
