"""Observe the real native SetStdHandle call on dbglist's freopen path."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import time
from case_paths import C2_SCRATCH, ROOT, native_work, windows

started=time.perf_counter()
fixture=Path('../fixtures/c2-debug-asm-listing')
phase=json.loads((fixture/'manifest.json').read_text())['phases']['c2']
assert len(phase['inputs'])==7 and len(phase['outputs'])==3
scratch=Path(tempfile.mkdtemp(prefix='stdhandle-native-',dir=C2_SCRATCH))
work,env=native_work(scratch,phase['canonical_working_directory'])
for row in phase['inputs']:
    data=(fixture/row['path']).read_bytes()
    assert data and hashlib.sha256(data).hexdigest()==row['sha256']
    (work/Path(row['path']).name).write_bytes(data)
env.update(PATH=os.defpath,LC_ALL='C',TZ='UTC',
    MSC_CMD_FLAGS=phase['environment']['MSC_CMD_FLAGS'].format(bin=windows(ROOT/'toolchains/1.0-nt/bin')),
    RECON_PROGRAM_PATH=windows(ROOT/'toolchains/1.0-nt/bin/C23232.EXE'))
executable=Path('build/native/C23232.EXE').resolve()
shared={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted((ROOT/'lib/recon/src').glob('*.c'))}
command=['gdb','-q','--batch','-ex','set pagination off','-ex','break parse_options',
    '-ex','run > native-inferior.stdout','-ex','break recon_SetStdHandle','-ex','continue',
    '-ex','printf "C2_IMPORT kind=%u handle=%#x before=%#x\\n",kind,handle,standard_handles[1]',
    '-ex','finish',
    '-ex','printf "C2_IMPORT_RESULT result=%u after=%#x\\n",$eax,standard_handles[1]',str(executable)]
setup=time.perf_counter()-started
phase_started=time.perf_counter()
result=subprocess.run(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
elapsed=time.perf_counter()-phase_started
log=result.stdout.decode(errors='replace')
Path('cases/dbglist-stdhandle/native.txt').write_bytes(result.stdout)
call=re.search(r'C2_IMPORT kind=(\d+) handle=(0x[0-9a-f]+|0) before=(0x[0-9a-f]+|0)',log)
returned=re.search(r'C2_IMPORT_RESULT result=(\d+) after=(0x[0-9a-f]+|0)',log)
assert result.returncode==0 and call and returned,log
observed=dict(std_handle=int(call[1]),handle=int(call[2],0),before_nonnull=bool(int(call[3],0)),result=bool(int(returned[1])),after_std=int(returned[2],0))
assert observed==dict(std_handle=0xfffffff5,handle=0,before_nonnull=True,result=True,after_std=0),observed
for path,value in shared.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==value,'Shared source changed during observation'
receipt=dict(observed=observed,executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),shared_hashes=shared,
    setup_seconds=setup,execution_seconds=elapsed,total_seconds=time.perf_counter()-started,
    transport='Inferior stdout is a disk file, matching the original reference debugger startup handle',command=command)
Path('cases/dbglist-stdhandle/native.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(observed)
