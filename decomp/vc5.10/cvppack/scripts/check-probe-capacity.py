#!/usr/bin/env python3
"""Compile the actual reference-helper parser/ownership guard as native i386."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
from state import BINARY,sha

start=time.perf_counter();source=(BINARY/'scripts/debug-run.c').read_text();pair=BINARY/'cases/hello-debug-dll/evidence/0109-fill-name-stream';out=pair/'probe-capacity-checks.json'
a=source.index('    value = getenv("VC510_CVPACK_PROBES");');b=source.index('    value = getenv("VC510_CVPACK_BLOBS");',a);parser=source[a:b]
a=source.index('    for(i=0;i<probe_count;++i) {',source.index('    fclose(plan);'));b=source.index('    probes_enabled =',a);guard=source[a:b]
assert 'probe_count < 256' in parser and 'if(value && *value) invalid_probe_configuration=1;' in parser and 'owner==range_count' in guard
fields=[]
for pattern in [r'static DWORD probe_sites\[256\].*?;',r'static char probe_names\[256\]\[64\].*?;']:
 matches=re.findall(pattern,source);assert len(matches)==1;fields+=matches
ranges=[]
for line in (pair/'original/runtime-plan').read_text().splitlines():
 first,last,_=[int(v,16) for v in line.split()];ranges.append((first,last))
program='#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\ntypedef unsigned long DWORD;\n'+ '\n'.join(fields)+'\nstatic unsigned probe_count=6;\nstatic int invalid_probe_configuration;\nstatic struct { DWORD start,end; } ranges[]={'+','.join('{%d,%d}'%r for r in ranges)+'};\nstatic DWORD range_count=sizeof(ranges)/sizeof(ranges[0]);\nint main(void) { char *value,*next; int consumed; DWORD probe_index,i; FILE *log=tmpfile();\n'+parser+'\nif(invalid_probe_configuration)return93;\n'.replace('return93','return 93')+guard+'\nfclose(log);return 0;}\n'
seed=set(int(v,16) for v in re.findall('0x[0-9a-f]+',fields[0]));points=[v for first,last in ranges for v in range(first,last) if v not in seed][:251];assert len(points)==251
configuration=lambda count:','.join('%x:probe%d'%(address,i) for i,address in enumerate(points[:count]))
checks=[('actual-needed134-slots',configuration(128),0),('exact-max256-slots',configuration(250),0),('excess257-slots',configuration(251),93),('duplicate-seeded-probe','%x:duplicate'%next(iter(seed)),93),('duplicate-configured-probe','%x:a,%x:b'%(points[0],points[0]),93),('malformed-address','ZZZ:probe',93),('missing-name','%x:'%points[0],93),('overlong-name','%x:'%points[0]+'a'*64,93),('trailing-comma',configuration(1)+',',93),('unknown-outside-unmasked-code','deadbeef:probe',93)]
results=[]
with tempfile.TemporaryDirectory(prefix='cvpack-probe-parser-',dir=Path.home()/'.ai-tmp') as folder:
 path=Path(folder);(path/'parser.c').write_text(program);compiled=subprocess.run(['gcc','-m32','-std=c89','-O0',str(path/'parser.c'),'-o',str(path/'parser')],capture_output=True,text=True);assert compiled.returncode==0,compiled.stderr
 for name,value,expected in checks:
  result=subprocess.run([str(path/'parser')],env=dict(os.environ,VC510_CVPACK_PROBES=value),capture_output=True);assert result.returncode==expected,(name,result.returncode,result.stderr);results.append(dict(name=name,status=expected))
report=dict(schema=1,native_only=True,scope='Exact parser and unmasked-range guard fragments extracted from the actual original-only helper, compiled natively as i386. No Wine/Windows child invocation. Actual fresh reference separately reports134 configured probes consumed completely under max256.',checks=results,helper_source_sha256=sha(BINARY/'scripts/debug-run.c'),parser_sha256=hashlib.sha256(parser.encode()).hexdigest(),guard_sha256=hashlib.sha256(guard.encode()).hexdigest(),native_harness_sha256=hashlib.sha256(program.encode()).hexdigest(),runtime_plan_sha256=sha(pair/'original/runtime-plan'),original_receipt_sha256=sha(pair/'original/receipt.json'),seconds=time.perf_counter()-start)
out.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=len(results),seconds=report['seconds'])))
