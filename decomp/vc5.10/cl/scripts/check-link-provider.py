#!/usr/bin/env python3
"""Check strict callback inputs/effects; actual native/GDB proof is separate."""
import copy
import json
from pathlib import Path
import shutil
import tempfile
import time
from case_inputs import dependencies,copy_dependencies
from link_provider import Provider,digest
PROJECT=Path(__file__).resolve().parents[1]
response=json.loads((PROJECT/'cases/link-object/dependency-packet-v2.json').read_bytes());reference=PROJECT/'cases/link-object/reference/original'
started=time.perf_counter();passed=[]
with tempfile.TemporaryDirectory(prefix='link-provider-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary);copy_dependencies(dependencies('link-object'),directory)
    for name in response['child']['provider_inputs']:shutil.copyfile(reference/name,directory/name)
    for name in response['child']['inputs']:shutil.copyfile(reference/'observer/link-input'/name,directory/name)
    provider=Provider.__new__(Provider);provider.directory=directory;provider.raw=reference;provider.response=response;provider.effects=[]
    provider.initial_files={path.name for path in directory.iterdir()}-{response['child']['argv'][2][3:]}
    request={'mode':0,'pid':123,'file':response['child']['path'],'cwd':str(directory),'argv':response['child']['argv'],
        'environment':response['child']['environment'],'environment_source':'target-os','parent_state':{'errno':0,'doserrno':2,'last_error':0}}
    def denied(label,change=None,file_change=None):
        altered=copy.deepcopy(request);saved=None;path=None
        if change:change(altered)
        if file_change:
            name,alter=file_change;path=directory/name;saved=path.read_bytes()if path.exists()else None
            path.write_bytes(alter(saved))
        try:
            try:provider._spawn(altered)
            except (AssertionError,KeyError):passed.append(label)
            else:raise AssertionError('accepted unknown LINK request: '+label)
            assert not(directory/'test.exe').exists() and provider.effects==[]
        finally:
            if path:
                if saved is None:path.unlink()
                else:path.write_bytes(saved)
    denied('wrong mode',lambda r:r.update(mode=1))
    denied('bool mode',lambda r:r.update(mode=False))
    denied('zero PID',lambda r:r.update(pid=0))
    denied('bool PID',lambda r:r.update(pid=True))
    denied('wrong executable',lambda r:r.update(file='c2.exe'))
    denied('wrong argv count',lambda r:r['argv'].append('/DEBUG'))
    denied('wrong response filename',lambda r:r['argv'].__setitem__(2,'@other.lk'))
    denied('wrong CWD',lambda r:r.update(cwd='/tmp'))
    denied('host environment leakage',lambda r:r['environment'].append('CL_HOST_CONTROL=1'))
    denied('environment order',lambda r:r['environment'].reverse())
    denied('wrong namespace',lambda r:r.update(environment_source='host'))
    denied('wrong LastError',lambda r:r['parent_state'].update(last_error=2))
    denied('wrong DOS error',lambda r:r['parent_state'].update(doserrno=0))
    denied('wrong errno',lambda r:r['parent_state'].update(errno=2))
    denied('bool error',lambda r:r['parent_state'].update(errno=False))
    denied('extra request field',lambda r:r.update(unknown=1))
    for name in ('test.obj',response['child']['argv'][2][3:],'LINK.EXE','MSVCRT.DLL','MSPDB50.DLL'):
        denied('changed input '+name,file_change=(name,lambda raw:bytes([raw[0]^1])+raw[1:]))
    denied('unknown input file',file_change=('extra.obj',lambda raw:b'unknown'))
    result=provider._spawn(copy.deepcopy(request));assert result=={'status':0,'stdout':b'','stderr':b'','parent_state':{'before':{'errno':0,'doserrno':2,'last_error':0},'after':{'errno':0,'doserrno':0,'last_error':0}}}
    assert digest((directory/'test.exe').read_bytes())==response['child']['output']['sha256']
    try:provider._spawn(copy.deepcopy(request))
    except AssertionError:passed.append('repeated invocation')
    else:raise AssertionError('accepted repeated LINK invocation')
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'scope':'Real owned input/artifact callback guards, not native child acceptance'}))
