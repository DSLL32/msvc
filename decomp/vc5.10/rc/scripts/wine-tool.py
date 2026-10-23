#!/usr/bin/env python3
"""Explicit original-toolchain build/capture transport; never imported by fixtures."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home()/'.ai-tmp/msvc2'
TC = ROOT/'toolchains/5.10'

def windows(path):
    path=Path(path).resolve()
    for base, drive in [(ROOT, 'R:'), (SCRATCH, 'S:')]:
        try: rel=path.relative_to(base)
        except ValueError: continue
        return drive+'\\'+str(rel).replace('/', '\\')
    raise ValueError('Path outside shared repo/scratch: '+str(path))

def environment():
    assert (PREFIX/'system.reg').is_file(), 'Shared prefix not initialized'
    for d, p in [('r:',ROOT),('s:',SCRATCH)]:
        assert (PREFIX/'dosdevices'/d).resolve()==p.resolve(), 'Drive mapping changed'
    env=dict(os.environ, WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
             WINEDLLOVERRIDES='msvcrt=n', WINEPATH=windows(TC/'bin'),
             INCLUDE=windows(TC/'include'), LIB=windows(TC/'lib'))
    for k in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS']:
        env.pop(k,None)
    return env

if __name__=='__main__':
    if len(sys.argv)<2: raise SystemExit('tool name required')
    raise SystemExit(subprocess.call(['wine',windows(TC/'bin'/sys.argv[1]),*sys.argv[2:]],env=environment()))
