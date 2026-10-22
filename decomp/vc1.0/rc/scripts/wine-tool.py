#!/usr/bin/env python3
"""Run the original NT toolchain in this reconstruction's private prefix."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/1.0-nt'
PREFIX = Path.home() / '.ai-tmp/vc1.0-rc/wine'

def windows(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')

def environment():
    env = os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               INCLUDE='T:\\include', LIB='T:\\lib')
    env['WINEPATH'] = 'T:\\bin'
    for name in ('CL', '_CL_', 'LINK', '_LINK_', 'MSC_CMD_FLAGS'):
        env.pop(name, None)
    return env

if __name__ == '__main__':
    if sys.argv[1] == '--init':
        if not (PREFIX / 'system.reg').exists():
            PREFIX.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(['wineboot', '-u'], env=environment(), check=True)
        drive = PREFIX / 'dosdevices/t:'
        if not drive.is_symlink(): drive.symlink_to(TC, target_is_directory=True)
        assert drive.resolve() == TC.resolve()
        server = subprocess.run(['wineserver', '-p'], env=environment(),
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        assert server.returncode in (0, 2), 'could not start private Wine server'
        sys.exit(0)
    sys.exit(subprocess.call(['wine', str(TC / 'bin' / sys.argv[1])] + sys.argv[2:],
                             env=environment()))
