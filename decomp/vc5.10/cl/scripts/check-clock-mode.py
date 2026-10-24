#!/usr/bin/env python3
"""Offline guard controls; no original, target acceptance or child execution."""
import copy,ctypes,json,struct,subprocess,tempfile
from pathlib import Path
from original import clock_analysis_mode
from accept import reconstruction_capture
PROJECT=Path(__file__).resolve().parents[1];passed=[]
assert clock_analysis_mode(True,'link-timing',False,False,False,False,True)=='CL_LINK_CLOCK_ANALYSIS_V1\n'
assert clock_analysis_mode(False,'help',False,False,False,False,False)is None
for index,label,value in [(0,'masked analysis',False),(1,'wrong case','link-object'),(2,'DLL analysis',True),(3,'DLL complete',True),(4,'DLL response',True),(5,'masked LINK response',True)]:
    args=[True,'link-timing',False,False,False,False,True];args[index]=value
    try:clock_analysis_mode(*args)
    except AssertionError:passed.append(label)
    else:raise AssertionError('accepted '+label)
for baseline in (True,False):
    try:reconstruction_capture({'baseline':baseline,'clock_analysis':True})
    except AssertionError:passed.append('clock analysis cannot be acceptance baseline='+str(baseline))
    else:raise AssertionError('clock analysis accepted')
source=(PROJECT/'scripts/clock-observer.inc').read_text()
with tempfile.TemporaryDirectory(prefix='clock-guards-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as name:
    directory=Path(name);c=directory/'guard.c';lib=directory/'guard.so'
    a=source.index('static int clock_call_bytes(');b=source.index('static void clock_install(',a)
    x=source.index('static int clock_point(');y=source.index('static void clock_trap(',x)
    declarations='typedef unsigned char BYTE;typedef unsigned DWORD;\nunsigned target_base=0x400000;DWORD clock_sites[2]={0x5402,0x5431};int clock_installed[2],clock_pending[2];int *clock_events;\n'
    c.write_text('#include <string.h>\n'+declarations+source[a:b].replace('static int','int')+source[x:y].replace('static int','int'))
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(c),'-o',str(lib)],check=True,capture_output=True)
    module=ctypes.CDLL(str(lib));fn=module.clock_call_bytes;fn.argtypes=[ctypes.c_char_p,ctypes.c_uint]
    mode=module.clock_observation_mode;assert mode(0,1,1)==1
    for label,arguments in [('masked helper clock marker',(1,1,1)),('wrong helper dependency family',(0,0,1)),('ordinary LINK response without clock-analysis',(0,1,0))]:
        assert mode(*arguments)==0;passed.append(label)
    call=b'\xff\x15'+struct.pack('<I',0x40a2a4);assert fn(call,0x400000)==1
    for label,raw in [('wrong opcode',b'\xfe'+call[1:]),('wrong addressing mode',call[:1]+b'\x14'+call[2:]),('wrong qualified IAT',call[:2]+struct.pack('<I',0x40a254))]:
        assert fn(raw,0x400000)==0;passed.append(label)
    assert fn(call,0x500000)==0;passed.append('different image base')
    installed=(ctypes.c_int*2).in_dll(module,'clock_installed');pending=(ctypes.c_int*2).in_dll(module,'clock_pending');events=ctypes.c_void_p.in_dll(module,'clock_events');events.value=1
    for i,site in enumerate((0x405402,0x405431)):
        assert module.clock_point(site)==-1;passed.append('uninstalled site '+str(i));installed[i]=1
        assert module.clock_point(site)==i
        assert module.clock_point(site+6)==-1;passed.append('unarmed return '+str(i));pending[i]=1
        assert module.clock_point(site+6)==i
        assert module.clock_point(site+7)==-1;passed.append('wrong return '+str(i))
assert 'if (call[0]==0xcc) continue;'in source
assert 'if (clock_calls!=(DWORD)point) ExitProcess(143);'in source
assert 'patch_byte(target_base+clock_sites[point],0xcc);'in source
assert 'if (context->Esp!=clock_stack[point]) ExitProcess(144);'in source
print(json.dumps({'passed':True,'negative_controls':len(passed),'controls':passed,'scope':'Compiled exact guard primitives; no Wine/target/acceptance execution'}))
