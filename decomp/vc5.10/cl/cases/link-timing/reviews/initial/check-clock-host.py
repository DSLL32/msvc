#!/usr/bin/env python3
"""Test the real uninstrumented clock host's mandatory finish inventory."""
import argparse,json,os,subprocess,tempfile,time
from pathlib import Path
from clock_provider import response_packet
from link_provider import binding,encoded,digest

PROJECT=Path(__file__).resolve().parents[1];ROOT=PROJECT.parents[2]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('original',type=Path);args=parser.parse_args()
response=response_packet(args.original);payload=binding(response);identity=digest(encoded(response));started=time.perf_counter();passed=[]
with tempfile.TemporaryDirectory(prefix='clock-host-',dir='/root/.ai-tmp/msvc2/vc5.10/cl')as temporary:
    directory=Path(temporary);source=directory/'host.c';binary=directory/'host';config=directory/'binding.bin';config.write_bytes(payload)
    source.write_text('''#define RECON_SHIM_IMPL
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <recon.h>
extern char **environ;
extern char **link_bind_entry(int,char **,char **);
int main(int argc,char **argv) {
 struct _timeb value;int n=atoi(getenv("TEST_CALLS"));
 link_bind_entry(argc,argv,environ);
 while(n--) recon_ftime(&value);
 return 0;
}
''')
    revision=subprocess.check_output(['git','rev-parse','2265dfaad'],cwd=ROOT,text=True).strip()
    shared=Path('/root/.ai-tmp/msvc2/vc5.10/cl/shared-source-checkouts')/revision/'lib/recon'
    sources=[p for p in(shared/'src').glob('*.c')if p.name!='trace.c']
    assert sources
    for p in sources:assert p.read_bytes()==subprocess.check_output(['git','show',revision+':lib/recon/src/'+p.name],cwd=ROOT)
    command=['clang','-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g','-DRECON_NATIVE_TEST','-DRECON_CRT_PROFILE_MSVCRT_510',
        '-I'+str(shared/'include'),'-I'+str(shared/'native'),str(source),str(PROJECT/'scripts/link-host.c'),*map(str,sources),'-o',str(binary)]
    compiled=subprocess.run(command,capture_output=True);assert compiled.returncode==0,compiled.stderr.decode()
    env=os.environ.copy();env.update(CL_ENTRY_PROFILE='link-timing-canonical-msvcrt-v1',CL_LINK_BINDING=str(config),
        CL_LINK_RESPONSE_ID=identity,RECON_LOCAL_CLOCK=response['clock']['clock_input'],TZ='UTC',TZDIR='/usr/share/zoneinfo')
    command=[str(binary),*['argument']*8]
    def run(expected,calls,change=None):
        values={**env,'CL_CLOCK_EXPECTED_CALLS':str(expected),'TEST_CALLS':str(calls)}
        if change:change(values)
        return subprocess.run(command,cwd=directory,env=values,capture_output=True,timeout=5)
    assert run(1,1).returncode==run(2,2).returncode==0
    for label,expected,calls in [('missing first',1,0),('missing both',2,0),('missing second',2,1),('extra first',1,2),('extra third',2,3),('unknown zero',0,0),('unknown third',3,3),('malformed count','x',0)]:
        result=run(expected,calls);assert result.returncode==125,(label,result.returncode,result.stderr);passed.append(label)
    for label,change in [('missing count',lambda e:e.pop('CL_CLOCK_EXPECTED_CALLS')),('missing clock',lambda e:e.pop('RECON_LOCAL_CLOCK')),('wrong profile',lambda e:e.update(CL_ENTRY_PROFILE='help-canonical-msvcrt-v1')),('unbound timing',lambda e:(e.pop('CL_LINK_BINDING'),e.pop('CL_LINK_RESPONSE_ID')))]:
        result=run(1,1,change);assert result.returncode==125,(label,result.returncode,result.stderr);passed.append(label)
print(json.dumps({'passed':True,'negative_controls':len(passed),'seconds':time.perf_counter()-started,
    'scope':'Actual i386 host+shared configured clock; no target reconstruction/Wine/dependency execution'}))
