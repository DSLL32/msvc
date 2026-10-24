#!/usr/bin/env python3
"""Explicit original builds/capture only. Never imported from native fixtures."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home() / '.ai-tmp/msvc2'
TC = ROOT / 'toolchains/5.10'


def windows(path):
    path = Path(path).resolve()
    for prefix, letter in [(ROOT, 'R:'), (SCRATCH, 'S:')]:
        if path.is_relative_to(prefix):
            return letter + '\\' + str(path.relative_to(prefix)).replace('/', '\\')
    raise ValueError('not within shared drives: ' + str(path))


def environment():
    if not (PREFIX / 'system.reg').exists():
        raise RuntimeError('shared prefix missing')
    for drive, target in [('r', ROOT), ('s', SCRATCH)]:
        if (PREFIX / 'dosdevices' / (drive + ':')).resolve() != target:
            raise RuntimeError('shared drive changed')
    result = os.environ.copy()
    result.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
                  WINEDLLOVERRIDES='msvcrt,mspdb50,msdis100,imagehlp=n',
                  WINEPATH=windows(TC / 'bin'), INCLUDE=windows(TC / 'include'),
                  LIB=windows(TC / 'lib'))
    for name in ['CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', 'LINK32_DB','LINK_DB','LINK_REPRO','LINK_REPRO_NAME']:
        result.pop(name, None)
    return result


if __name__ == '__main__':
    raise SystemExit(subprocess.call(['wine', windows(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                                    env=environment()))
