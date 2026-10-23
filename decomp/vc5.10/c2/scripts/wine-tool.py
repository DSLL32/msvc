#!/usr/bin/env python3
"""Explicit original-build transport only; never a native fixture dependency."""
import os
from pathlib import Path
import subprocess
import sys

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
TC = ROOT / 'toolchains/5.10'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home() / '.ai-tmp/msvc2'

def windows(path):
    path = Path(path).resolve()
    for base, drive in ((ROOT, 'R:'), (SCRATCH, 'S:')):
        if path == base or base in path.parents:
            return drive + '\\' + str(path.relative_to(base)).replace('/', '\\')
    raise ValueError('Path outside fixed repo/scratch mapping: '+str(path))

def environment():
    assert (PREFIX/'dosdevices/r:').resolve() == ROOT
    assert (PREFIX/'dosdevices/s:').resolve() == SCRATCH
    env = dict(os.environ, WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               WINEDLLOVERRIDES='msvcrt=n', INCLUDE=windows(TC/'include'),
               LIB=windows(TC/'lib'), WINEPATH=windows(TC/'bin'))
    for key in ('CL', '_CL_', 'MSC_CMD_FLAGS', '_MSC_IDE_FLAGS'):
        env.pop(key, None)
    return env

if __name__ == '__main__':
    result = subprocess.run(['/usr/bin/wine', windows(TC/'bin'/sys.argv[1]), *sys.argv[2:]],
                            cwd=BINARY, env=environment(), timeout=45)
    sys.exit(result.returncode)
