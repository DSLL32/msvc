#!/usr/bin/env python3
"""Offline fresh-clock mode controls; no Wine or target execution."""
import ctypes
import json
from pathlib import Path
import subprocess
import tempfile
from original import clock_response_mode
from accept import reconstruction_capture
from clock_packet import decode

PROJECT=Path(__file__).resolve().parents[1];passed=[]
config={'accepted':['_start_timer','_link_pass','_write_link_response','_write_link_message','_execute_pass']}
args=[config,False,'link-timing',False,False,False,False,False,True]
assert clock_response_mode(*args)=='CL_LINK_CLOCK_RESPONSE_V1\n'
for index,label,changed in [(1,'unmasked',True),(2,'wrong case','link-object'),(3,'DLL analysis',True),
        (4,'DLL complete',True),(5,'DLL response',True),(6,'ordinary LINK response',True),(7,'clock analysis',True)]:
    candidate=list(args);candidate[index]=changed
    try:clock_response_mode(*candidate)
    except AssertionError:passed.append(label)
    else:raise AssertionError('accepted '+label)
for name in config['accepted']:
    candidate=list(args);candidate[0]={'accepted':[n for n in config['accepted']if n!=name]}
    try:clock_response_mode(*candidate)
    except AssertionError:passed.append('unaccepted caller '+name)
    else:raise AssertionError('enabled unaccepted '+name)
assert clock_response_mode(config,True,'help',False,False,False,False,False,False)is None
for baseline in (True,False):
    try:reconstruction_capture({'clock_response':True,'baseline':baseline})
    except AssertionError:passed.append('incomplete clock receipt rejected baseline='+str(baseline))
    else:raise AssertionError('incomplete clock response accepted')
source=(PROJECT/'scripts/clock-observer.inc').read_text()
with tempfile.TemporaryDirectory(prefix='clock-response-mode-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as name:
    directory=Path(name);c=directory/'guard.c';lib=directory/'guard.so'
    a=source.index('static int clock_response_observation_mode(');b=source.index('static void clock_install(',a)
    c.write_text(source[a:b].replace('static int','int'))
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(c),'-o',str(lib)],check=True,capture_output=True)
    mode=ctypes.CDLL(str(lib)).clock_response_observation_mode
    assert mode(1,1,0,1)==1
    for masked in (0,1):
        for family in (0,1):
            for analysis in (0,1):
                for response in (0,1):
                    values=(masked,family,analysis,response)
                    if values==(1,1,0,1):continue
                    assert mode(*values)==0;passed.append('compiled helper mode '+str(values))
assert 'if (call[0]==0xcc) continue;'in source
assert 'if (clock_calls!=(DWORD)point) ExitProcess(143);'in source
assert 'if (context->Esp!=clock_stack[point]) ExitProcess(144);'in source
assert 'patch_byte(target_base+clock_sites[point],0xcc);'in source
baseline=Path('/root/.ai-tmp/msvc2/vc5.10/cl/link-clock-analysis-1')
try:decode(baseline,response=True)
except AssertionError:passed.append('actual baseline cannot supply fresh response')
else:raise AssertionError('baseline relabeled as response')
print(json.dumps({'passed':True,'negative_controls':len(passed),'controls':passed,
    'scope':'Exact controller/compiled C mode guards and baseline rejection; complete typed fresh native proof remains mandatory'}))
