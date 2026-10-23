#!/usr/bin/env python3
"""Offline checks for masked LINK observation, owned paths and acceptance hold."""
import copy
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from original import link_mode
from accept import reconstruction_capture

PROJECT=Path(__file__).resolve().parents[1]
config=json.loads((PROJECT/'functions.json').read_text());config['accepted'].append('_write_link_message')
assert link_mode(config,False,'link-object',False,False,False,True)=='CL_LINK_RESPONSE_V1\n'
passed=[]
def denied(name,callback):
    try:callback()
    except (AssertionError,KeyError):passed.append(name)
    else:raise AssertionError('accepted invalid input: '+name)
for name,arguments in [
    ('baseline',(config,True,'link-object',False,False,False,True)),
    ('wrong case',(config,False,'minimal-c',False,False,False,True)),
    ('analysis',(config,False,'link-object',True,False,False,True)),
    ('complete analysis',(config,False,'link-object',False,True,False,True)),
    ('other response',(config,False,'link-object',False,False,True,True))]:
    denied(name,lambda arguments=arguments:link_mode(*arguments))
for missing in ('_link_pass','_write_link_response','_write_link_message','_execute_pass'):
    value=copy.deepcopy(config);value['accepted'].remove(missing)
    denied('unaccepted '+missing,lambda value=value:link_mode(value,False,'link-object',False,False,False,True))
marker=b'CL_LINK_RESPONSE_V1\n'
capture={'baseline':False,'dependency_analysis':False,'dependency_complete':False,'dependency_response':False,
    'link_response':True,'controller_error':None,'helper_status':0,'case':'link-object',
    'files':{'dependency-observer.txt':{'bytes':len(marker),'sha256':hashlib.sha256(marker).hexdigest()}},'command':['helper','1','out']}
denied('valid observation cannot bypass missing comparator/provider',lambda:reconstruction_capture(capture))
for name,change in [('wrong marker',lambda c:c['files']['dependency-observer.txt'].update(sha256='0'*64)),
                    ('unmasked helper',lambda c:c.update(command=['helper','0','out'])),
                    ('wrong child family',lambda c:c.update(case='minimal-c'))]:
    changed=copy.deepcopy(capture);change(changed);denied(name,lambda changed=changed:reconstruction_capture(changed))
source=(PROJECT/'scripts/dependency-observer.inc').read_text()
start=source.index('static int dependency_link_paths(');end=source.index('static void dependency_install(',start)
with tempfile.TemporaryDirectory(prefix='cl-link-paths-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary);c=directory/'paths.c';library=directory/'paths.so'
    c.write_text('#include <string.h>\n#include <stdio.h>\ntypedef unsigned DWORD;\n'+source[start:end].replace('static int dependency_link_paths','int dependency_link_paths'))
    subprocess.run(['cc','-shared','-fPIC','-Wall','-Wextra','-Werror',str(c),'-o',str(library)],check=True,capture_output=True)
    check=ctypes.CDLL(str(library)).dependency_link_paths;check.argtypes=[ctypes.c_char_p,ctypes.c_uint,ctypes.c_char_p,ctypes.c_uint]
    def run(value,capacity=260,destination_capacity=260):
        original=value.encode();response=ctypes.create_string_buffer(original,260);destination=ctypes.create_string_buffer(260)
        result=check(response,capacity,destination,destination_capacity)
        return result,response.value,destination.value
    assert run('.\\a01234')==(1,b'.\\a01234lk',b'observer\\link-input\\a01234lk')
    for value in ('','.', '.\\', 'a01234', '.\\a0123', '.\\a012345', '..a01234', '.\\../abc', '.\\a0:234', '.\\a0/234', '.\\a0\\234'):
        assert run(value)[0]==0;passed.append('invalid owned prefix '+repr(value))
    assert run('.\\a01234',10)[0]==0;passed.append('no response suffix capacity')
    assert run('.\\a01234',260,len(b'observer\\link-input\\a01234lk'))[0]==0;passed.append('no destination terminator capacity')
assert 'if (dependency_link_response && i!=5 && i!=7 && i!=8) continue;'in source
assert 'observed-child family=link'in source
print(json.dumps({'passed':True,'negative_controls':len(passed),'scope':'No Wine, child execution, fixture or function acceptance','controls':passed}))
