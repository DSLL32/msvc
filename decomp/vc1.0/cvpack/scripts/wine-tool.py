#!/usr/bin/env python3
"""Explicit original compiler/reference execution, never imported by fixtures."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/1.0-nt'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'

def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')

def environment():
    assert (PREFIX / 'system.reg').is_file(), 'shared root prefix must already exist'
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               WINESERVER='/usr/lib/wine/wineserver64',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'),
               WINEPATH=windows(TC / 'bin'))
    for name in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', 'LINK32_DB'):
        env.pop(name, None)
    return env

if __name__ == '__main__':
    sys.exit(subprocess.call(['wine', str(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                             env=environment()))
