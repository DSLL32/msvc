#!/usr/bin/env python3
"""Exercise the native-only namespace binder with real shared CRT storage."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time
from link_provider import binding,encoded

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
response=json.loads((PROJECT/'cases/link-object/dependency-packet-v2.json').read_bytes());payload=binding(response);identity=hashlib.sha256(encoded(response)).hexdigest()
started=time.perf_counter();passed=[]
with tempfile.TemporaryDirectory(prefix='link-host-controls-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary);source=directory/'host.c';binary=directory/'host';config=directory/'binding.bin'
    source.write_text('''#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <recon.h>
extern char **environ;
extern char **link_bind_entry(int,char **,char **);
extern char **link_initial_environment;
extern WCHAR *link_initial_os_environment;
int main(int argc,char **argv) {
 char **env=link_bind_entry(argc,argv,environ);
 if (!recon_crt_arguments_bound || env!=link_initial_environment || env!=recon_crt_environment || env!=recon_crt_initenv ||
     recon_crt_os_environment!=link_initial_os_environment || recon_crt_environment_updates) return 90;
 if(getenv("REPEAT_BIND")) link_bind_entry(argc,argv,environ);
 puts("bound");return 0;
}
''')
    revision=subprocess.check_output(['git','rev-parse','094d818f'],cwd=ROOT,text=True).strip()
    shared=Path('/root/.ai-tmp/msvc2/vc5.10/cl/shared-source-checkouts')/revision/'lib/recon'
    sources=[path for path in(shared/'src').glob('*.c')if path.name!='trace.c']
    for path in sources:
        assert path.read_bytes()==subprocess.check_output(['git','show',revision+':lib/recon/src/'+path.name],cwd=ROOT)
    command=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST','-DRECON_CRT_PROFILE_MSVCRT_510',
        '-I'+str(shared/'include'),'-I'+str(shared/'native'),str(source),str(PROJECT/'scripts/link-host.c'),*map(str,sources),'-o',str(binary)]
    compiled=subprocess.run(command,cwd=PROJECT,capture_output=True)
    assert compiled.returncode==0,compiled.stderr.decode()
    environment=os.environ.copy();environment.update(RECON_SHARED_REVISION='094d818f',CL_ENTRY_PROFILE='link-object-canonical-msvcrt-v1',CL_LINK_BINDING=str(config),CL_LINK_RESPONSE_ID=identity)
    args=[str(binary),*response['child']['argv'], 'a','b','c','d']
    assert len(args)==8
    def run(data=payload,change=None):
        config.write_bytes(data);env=environment.copy()
        if change:change(env)
        return subprocess.run(args,cwd=directory,env=env,capture_output=True,timeout=5)
    result=run();assert result.returncode==0 and result.stdout==b'bound\n',result.stderr
    def denied(label,data=payload,change=None):
        result=run(data,change);assert result.returncode==125,(label,result.returncode,result.stderr);passed.append(label)
    denied('unknown magic',b'BAD!'+payload[4:])
    denied('unknown version',payload[:4]+struct.pack('<I',2)+payload[8:])
    denied('different response identity',payload[:8]+b'0'*64+payload[72:])
    denied('empty environment vector',payload[:72]+struct.pack('<I',0)+payload[76:])
    denied('oversized environment vector',payload[:72]+struct.pack('<I',4097)+payload[76:])
    denied('oversized string',payload[:76]+struct.pack('<I',32768)+payload[80:])
    denied('truncated string/vector',payload[:90])
    denied('truncated OS block',payload[:-2])
    denied('extra trailing bytes',payload+b'X')
    denied('wrong case entry profile',change=lambda env:env.update(CL_ENTRY_PROFILE='help-canonical-msvcrt-v1'))
    denied('missing identity',change=lambda env:env.pop('CL_LINK_RESPONSE_ID'))
    denied('missing binding',change=lambda env:env.pop('CL_LINK_BINDING'))
    denied('unbound LINK case',change=lambda env:(env.pop('CL_LINK_BINDING'),env.pop('CL_LINK_RESPONSE_ID')))
    denied('repeated binding',change=lambda env:env.update(REPEAT_BIND='1'))
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,'scope':'Actual i386 shared CRT binder, no target execution/Wine/child dependency'}))
