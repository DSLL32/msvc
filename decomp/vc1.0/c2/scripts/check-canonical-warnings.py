"""Compile selected modules with original CL /W3; reject actual warnings/errors."""
import json
from pathlib import Path
import re
import subprocess
import sys
import time
from case_paths import C2_SCRATCH

started = time.perf_counter()
rows = []
for module in sys.argv[1:]:
    assert re.fullmatch('[a-z][a-z0-9-]*',module)
    source = Path('src')/(module+'.c')
    assert source.is_file()
    command = ['python3','scripts/wine-tool.py','CL.EXE','/nologo','/I../../../lib/recon/include',
        '/c','/Od','/G3','/W3','/Fobuild/w3-'+module+'.obj',str(source)]
    phase = time.perf_counter()
    result = subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
    (C2_SCRATCH/('w3-'+module+'.log')).write_bytes(result.stdout)
    assert result.returncode == 0 and not re.search(rb'\b(?:warning|error) C[0-9]+',result.stdout),result.stdout
    rows.append(dict(module=module,seconds=time.perf_counter()-phase,returncode=result.returncode))
Path('cases/shared-prefix-warnings.json').write_text(json.dumps(dict(modules=rows,seconds=time.perf_counter()-started),indent=2)+'\n')
print(rows)
