#!/usr/bin/env python3
"""Native module identities and frozen original DisableThreadLibraryCalls probe."""
import json
from pathlib import Path
import subprocess
import sys
import time

start=time.perf_counter()
for argument in sys.argv[1:]:
    run=time.perf_counter()
    result=subprocess.run([str(Path(argument).resolve())],stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
    assert result.returncode==0 and not result.stderr,(argument,result)
    if 'module-probe' in argument:
        assert result.stdout==Path(__file__).with_name('module-probe.stdout').read_bytes(),result.stdout
    else:
        assert not result.stdout,result.stdout
    print(json.dumps(dict(executable=argument,status='passed',execution_seconds=time.perf_counter()-run)))
print(json.dumps(dict(total_seconds=time.perf_counter()-start)))
