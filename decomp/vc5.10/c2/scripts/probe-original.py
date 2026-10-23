#!/usr/bin/env python3
"""Explicit, original-only diagnostic capture. Never imported by fixtures."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path(__file__).resolve().parents[4]
CASE = ROOT / 'decomp/vc5.10/c2/cases/original-help'
WORK = Path.home() / '.ai-tmp/msvc2/vc5.10/c2/probes/help'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'

def main():
    CASE.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, WINEPREFIX=str(PREFIX), WINEDEBUG='-all')
    for key in ('CL', '_CL_', 'MSC_CMD_FLAGS'):
        env.pop(key, None)
    args = ['wine', str(ROOT / 'toolchains/5.10/bin/C2.EXE'), '/?']
    start = time.monotonic()
    run = subprocess.run(args, cwd=WORK, env=env, capture_output=True, timeout=45)
    elapsed = time.monotonic() - start
    (CASE / 'stdout.bin').write_bytes(run.stdout)
    (CASE / 'stderr.bin').write_bytes(run.stderr)
    receipt = dict(kind='original-only-unmasked-diagnostic', argv=args,
                   cwd=str(WORK), wineprefix=str(PREFIX), exit_status=run.returncode,
                   elapsed_seconds=elapsed, target_sha256=hashlib.sha256(Path(args[1]).read_bytes()).hexdigest(),
                   stdout_size=len(run.stdout), stderr_size=len(run.stderr),
                   stdout_sha256=hashlib.sha256(run.stdout).hexdigest(),
                   stderr_sha256=hashlib.sha256(run.stderr).hexdigest(),
                   acceptance=False, no_native_oracle=True)
    (CASE / 'capture.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))

if __name__ == '__main__':
    main()
