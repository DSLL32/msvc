#!/usr/bin/env python3
"""Run explicit original-toolchain builds in the repository shared Wine prefix."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/1.0-nt'
from case_paths import PREFIX, SCRATCH, windows

def environment():
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'))
    env['WINEPATH'] = windows(TC / 'bin')
    env.pop('CL', None)
    env.pop('_CL_', None)
    return env

if __name__ == '__main__':
    if sys.argv[1] == '--init':
        assert (PREFIX/'system.reg').exists(), 'Shared Wine prefix must be provisioned by owner'
        assert (PREFIX/'dosdevices/r:').resolve()==ROOT
        assert (PREFIX/'dosdevices/s:').resolve()==SCRATCH
        sys.exit(0)
    sys.exit(subprocess.call(['wine', windows(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                             env=environment()))
