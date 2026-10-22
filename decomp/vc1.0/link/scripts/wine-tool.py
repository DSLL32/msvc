#!/usr/bin/env python3
"""Run the original NT toolchain in this reconstruction's private prefix."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/1.0-nt'
PREFIX = Path.home() / '.ai-tmp/vc1.0-link/wine'

def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')

def environment():
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'))
    env['WINESERVER'] = '/usr/lib/wine/wineserver64'
    env['WINEPATH'] = windows(TC / 'bin')
    env.pop('CL', None)
    env.pop('_CL_', None)
    return env

if __name__ == '__main__':
    if sys.argv[1] == '--init':
        if not (PREFIX / 'system.reg').exists():
            PREFIX.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(['wineboot', '-u'], env=environment(), check=True)
        subprocess.run(['wineserver', '-p'], env=environment(),
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        sys.exit(0)
    sys.exit(subprocess.call(['wine', str(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                             env=environment()))
