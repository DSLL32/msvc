#!/usr/bin/env python3
"""Compare the native CRT wildcard boundary with a frozen original probe."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time

start = time.perf_counter()
binary = Path(sys.argv[1]).resolve()
scratch = Path.home() / '.ai-tmp/msvc2/recon-maintenance/find-tests'
scratch.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=scratch) as directory:
    work = Path(directory)
    # Reverse creation order deliberately differs from the captured enumeration.
    for name in ['unit1.obj', 'unit0.obj', 'UnitZ.OBJ', 'unit.obj', 'plain']:
        (work / name).write_bytes(b'test\n')
    (work / 'objects').mkdir()
    for name in ['unit1.obj', 'unit0.obj']:
        (work / 'objects' / name).write_bytes(b'test\n')
    setup = time.perf_counter() - start
    run = time.perf_counter()
    result = subprocess.run([str(binary)], cwd=work, stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
    execution = time.perf_counter() - run
    check = time.perf_counter()
    expected = Path(__file__).with_name('find-probe.stdout').read_bytes()
    assert result.returncode == 0 and not result.stderr, result
    assert result.stdout == expected, (result.stdout, expected)
    print(json.dumps(dict(status='passed', setup_seconds=setup,
        execution_seconds=execution, checks_seconds=time.perf_counter()-check,
        total_seconds=time.perf_counter()-start)))
