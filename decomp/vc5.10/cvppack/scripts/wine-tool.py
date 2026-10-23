#!/usr/bin/env python3
"""Explicit original compiler/check invocation; never imported by fixtures."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/5.10'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home() / '.ai-tmp/msvc2'


def windows(path):
    path = Path(path).resolve()
    for root, letter in ((ROOT, 'R'), (SCRATCH, 'S')):
        if path.is_relative_to(root):
            return letter + ':\\' + str(path.relative_to(root)).replace('/', '\\')
    raise ValueError('original paths must use shared R:/S: namespace: ' + str(path))


def environment():
    for letter, root in (('r', ROOT), ('s', SCRATCH)):
        assert (PREFIX / ('dosdevices/' + letter + ':')).resolve() == root.resolve()
    assert (PREFIX / 'system.reg').is_file()
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               WINEDLLOVERRIDES='msvcrt,mspdb50=n',
               INCLUDE=windows(TC / 'include'), LIB=windows(TC / 'lib'),
               WINEPATH=windows(TC / 'bin'))
    for key in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS', '_MSC_IDE_FLAGS', 'LINK32_DB'):
        env.pop(key, None)
    return env


if __name__ == '__main__':
    sys.exit(subprocess.call(['wine', windows(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                             env=environment()))
