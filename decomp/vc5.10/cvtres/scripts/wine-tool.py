#!/usr/bin/env python3
"""Explicit original compiler/reference execution; never a native dependency."""
import os
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[4]
TC = ROOT/'toolchains/5.10'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'
SCRATCH = Path.home()/'.ai-tmp/msvc2'

def windows(path):
    path=Path(path).resolve()
    for drive,base in [('R:',ROOT),('S:',SCRATCH)]:
        if path.is_relative_to(base):
            return drive+'\\'+str(path.relative_to(base)).replace('/','\\')
    raise ValueError('outside fixed repo/scratch mappings: '+str(path))

def environment():
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT
    assert (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    env=os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX),WINEARCH='win32',WINEDEBUG='-all',
        WINESERVER='/usr/lib/wine/wineserver64', WINEDLLOVERRIDES='msvcrt=n',
        INCLUDE=windows(TC/'include'), LIB=windows(TC/'lib'), WINEPATH=windows(TC/'bin'))
    for name in ['CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB']:
        env.pop(name,None)
    return env
if __name__=='__main__':
    raise SystemExit(subprocess.call(['wine',str(TC/'bin'/sys.argv[1]),*sys.argv[2:]],env=environment()))
