#!/usr/bin/env python3
"""Run explicit original builds in the repository shared Wine prefix."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/1.0-nt'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home()/'.ai-tmp/msvc2'
def windows(path):
    path = Path(path).resolve()
    for drive, root in [('R:', ROOT), ('S:', SCRATCH)]:
        try: return drive+'\\'+str(path.relative_to(root)).replace('/', '\\')
        except ValueError: pass
    raise ValueError('Path is outside fixed R:/S: roots: '+str(path))

def environment():
    assert (PREFIX/'system.reg').exists(), 'Shared prefix is managed externally'
    assert (PREFIX/'dosdevices/r:').resolve() == ROOT
    assert (PREFIX/'dosdevices/s:').resolve() == SCRATCH
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'))
    env['WINEPATH'] = windows(TC / 'bin')
    env.pop('CL', None)
    env.pop('_CL_', None)
    return env

if __name__ == '__main__':
    if sys.argv[1] == '--init':
        environment()
        sys.exit(0)
    sys.exit(subprocess.call(['wine', windows(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                             env=environment()))
