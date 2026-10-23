#!/usr/bin/env python3
"""Validate the pointer routine's native unsupported-offset hold, not an oracle."""
import json
from pathlib import Path
import resource
import signal
import subprocess
import tempfile
import time
from state import BINARY,ROOT,sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0076-compact-pointer-types-corrected-selector'
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o')
shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/pointer-gap.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-pointer-gap-',dir=Path.home()/'.ai-tmp') as temporary:
 executable=Path(temporary)/'pointer-gap'
 command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
 before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
 assert not built.returncode,built.stdout.decode()
 assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
 def no_core():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
 good=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,preexec_fn=no_core,timeout=5)
 assert good.returncode==0,(good.returncode,good.stderr)
 bad=subprocess.run([str(executable),'unsupported'],stdout=subprocess.PIPE,stderr=subprocess.PIPE,preexec_fn=no_core,timeout=5)
 assert bad.returncode==-signal.SIGTRAP,(bad.returncode,bad.stderr)
 debug=subprocess.run(['gdb','-q','-nx','-batch','-ex','run','-ex','x/i $pc-1','-ex','bt 3','--args',str(executable),'unsupported'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=5)
 actual=debug.stdout.decode();assert 'SIGTRAP' in actual and 'compact_legacy_pointer' in actual and 'type-pointers.c' in actual and 'int3' in actual,actual
 report=dict(schema=1,native_only=True,scope='Unknown embedded symbol kind stops before consuming original unwritten offset; no original unsupported-path equivalence claimed.',positive_near32_status=good.returncode,unsupported_kind=65535,negative_signal='SIGTRAP',negative_status=bad.returncode,gdb=actual,inputs=inputs,command=command,checker_sha256=sha(Path(__file__)),seconds=dict(build=build_seconds,total=time.perf_counter()-start))
 (pair/'pointer-gap-check.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(negative_signal='SIGTRAP',seconds=report['seconds'])))
