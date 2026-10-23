#!/usr/bin/env python3
"""One actual flag object for allocator/legacy/modern consumers, Linux only."""
import json
from pathlib import Path
import subprocess
import tempfile
import time
from binary import COFF, PE
from state import BINARY,ROOT,sha
start=time.perf_counter();pair=BINARY/'cases/hello-debug-dll/evidence/0077-rewrite-type-name-symbol-tail-proof'
data=json.loads((BINARY/'data.json').read_text());flag=[row for row in data if row['original']==0x412d00]
assert len(flag)==1 and flag[0]['name']=='discard_types' and flag[0]['size']==4
symbols=subprocess.check_output(['nm','-S','--defined-only',str(BINARY/'build/native/CVPACK')],text=True)
assert sum(line.split()[-1]=='discard_types' for line in symbols.splitlines())==1
assert 'global_types_exhausted' not in symbols
reference=PE(ROOT/'toolchains/5.10/bin/CVPACK.EXE');instructions=[]
for address,raw in [(0x40b493,'a1002d4100'),(0x40b866,'c705002d410001000000'),(0x4027fa,'392d002d4100'),(0x4031d0,'391d002d4100'),(0x403268,'391d002d4100')]:
 assert reference.read(address,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 instructions.append(dict(address=address,bytes=raw))
audit=json.loads((pair/'original/audit.json').read_text());assert len(audit['discard_flag_references'])==3
objects=sorted(path for path in (BINARY/'build/native').glob('*.o') if path.name!='driver.o');shims=sorted((ROOT/'lib/recon/src').glob('*.c'));source=BINARY/'scripts/type-flag.c'
inputs={str(path):sha(path) for path in objects+shims+[source,BINARY/'scripts/native-widen.c']}
with tempfile.TemporaryDirectory(prefix='cvpack-type-flag-',dir=Path.home()/'.ai-tmp') as temporary:
 executable=Path(temporary)/'type-flag'
 command=['clang','-DRECON_NATIVE_TEST','-I'+str(ROOT/'lib/recon/include'),'-I'+str(ROOT/'lib/recon/native'),'-I'+str(BINARY/'src'),'-m32','-std=gnu89','-fms-extensions','-fasm-blocks','-O0','-g',str(source),*map(str,objects),str(BINARY/'scripts/native-widen.c'),*map(str,shims),'-o',str(executable)]
 before=time.perf_counter();built=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);build_seconds=time.perf_counter()-before
 assert not built.returncode,built.stdout.decode();assert all(sha(path)==digest for path,digest in inputs.items()),'unit build inputs changed'
 result=subprocess.run([str(executable)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=5);assert result.returncode==0,(result.returncode,result.stderr)
 report=dict(schema=1,native_only=True,scope='Native nonzero shared-field identity control, not a new original nonzero-path/default oracle.',original_rva=0x12d00,original_instructions=instructions,actual_coff_references=audit['discard_flag_references'],native_status=result.returncode,consumers=['allocate_global_type','compact_type_index','compact_legacy_index'],inputs=inputs,command=command,checker_sha256=sha(Path(__file__)),seconds=dict(build=build_seconds,total=time.perf_counter()-start))
 (pair/'type-flag-check.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(nonzero_consumers=3,seconds=report['seconds'])))
