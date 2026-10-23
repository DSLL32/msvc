#!/usr/bin/env python3
"""Explicit original-compiler execution; never imported by native fixtures."""
import os
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[4]
TC = ROOT / 'toolchains/5.10'
PREFIX = Path.home() / '.ai-tmp/vc1.0-capture/wine'
def windows(path):
    path=Path(path).resolve()
    try: return 'R:\\'+str(path.relative_to(ROOT)).replace('/','\\')
    except ValueError: pass
    try: return 'S:\\'+str(path.relative_to(Path.home()/'.ai-tmp/msvc2')).replace('/','\\')
    except ValueError: raise ValueError('original tool path outside fixed R/S roots')
def environment():
    env=os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX), WINEARCH='win32', WINEDEBUG='-all',
               WINESERVER='/usr/lib/wine/wineserver64', WINEDLLOVERRIDES='msvcrt=n',
               INCLUDE=windows(TC/'include'),LIB=windows(TC/'lib'),WINEPATH=windows(TC/'bin'))
    for name in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','LINK32_DB'): env.pop(name,None)
    return env
if __name__=='__main__':
    sys.exit(subprocess.call(['wine',str(TC/'bin'/sys.argv[1])]+sys.argv[2:],env=environment()))
