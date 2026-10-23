#!/usr/bin/env python3
"""Explicit unmodified-original probes; never imported by native fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parents[1]
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home() / '.ai-tmp/msvc2/vc5.10/rc'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', choices=['help', 'minimal', 'resources-rich'])
    parser.add_argument('--replace', action='store_true', help='Explicitly replace an existing groundwork probe')
    args = parser.parse_args()
    if (HERE / 'cases' / args.case / 'probe' / 'receipt.json').exists() and not args.replace:
        parser.error('Probe exists; use --replace for an explicit recapture')
    for drive, target in [('r:', ROOT), ('s:', Path.home() / '.ai-tmp/msvc2')]:
        if (PREFIX / 'dosdevices' / drive).resolve() != target.resolve():
            raise SystemExit('Shared prefix mapping differs; refusing to remap it')
    scratch = SCRATCH / args.case
    scratch.mkdir(parents=True, exist_ok=True)
    if args.case != 'help':
        for path in (HERE / 'cases' / args.case / 'source').rglob('*'):
            if path.is_file():
                dest = scratch / path.relative_to(HERE / 'cases' / args.case / 'source')
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, dest)
    out = scratch / 'output.res'
    out.unlink(missing_ok=True)
    argv = ['/help'] if args.case == 'help' else ['/fooutput.res']
    if args.case == 'resources-rich':
        argv += ['/iinclude', '/dCASE_FROM_COMMAND=7', r'/iR:\toolchains\5.10\include', 'resources.rc']
    elif args.case == 'minimal':
        argv += ['minimal.rc']
    exe = ROOT / 'toolchains/5.10/bin/RC.EXE'
    env = dict(os.environ, WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all')
    for key in ['INCLUDE', 'LIB', 'CL', '_CL_']:
        env.pop(key, None)
    start = time.monotonic()
    timed_out = False
    try:
        result = subprocess.run(['wine', str(exe), *argv], cwd=scratch,
                                env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, timeout=30)
    except subprocess.TimeoutExpired as exc:
        timed_out = True
        result = SimpleNamespace(returncode=None, stdout=exc.stdout or b'',
                                 stderr=exc.stderr or b'')
    elapsed = time.monotonic() - start
    dest = HERE / 'cases' / args.case / 'probe'
    dest.mkdir(parents=True, exist_ok=True)
    (dest / 'stdout.bin').write_bytes(result.stdout)
    (dest / 'stderr.bin').write_bytes(result.stderr)
    if out.exists():
        shutil.copy2(out, dest / 'output.res')
    receipt = {'kind': 'unmodified-original-groundwork-probe',
        'not_reconstruction_acceptance': True, 'case': args.case,
        'argv': argv, 'executable_sha256': sha(exe),
        'rcdll_sha256': sha(ROOT / 'toolchains/5.10/bin/RCDLL.DLL'),
        'prefix': str(PREFIX), 'cwd': str(scratch), 'elapsed_seconds': elapsed,
        'exit_status': result.returncode, 'timed_out': timed_out,
        'streams': {name: {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                    for name, data in [('stdout', result.stdout), ('stderr', result.stderr)]},
        'inputs': {str(p.relative_to(scratch)): sha(p) for p in scratch.rglob('*')
                   if p.is_file() and p != out and p.name not in ['debug.stdout', 'debug.stderr']},
        'output': {'bytes': out.stat().st_size, 'sha256': sha(out)} if out.exists() else None}
    (dest / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))

if __name__ == '__main__':
    main()
