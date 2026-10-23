#!/usr/bin/env python3
"""Explicit original builds/captures in the root-managed shared Wine prefix."""
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0,str(Path(__file__).resolve().parent))
from case_paths import PREFIX, ROOT, SCRATCH, windows
TC=ROOT/'toolchains/1.0-nt'


def validate_prefix():
    assert (PREFIX/'system.reg').exists(),'Shared prefix must already be initialized by root'
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT
    assert (PREFIX/'dosdevices/s:').resolve()==SCRATCH


def environment():
    validate_prefix()
    env=os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX),WINEARCH='win32',WINEDEBUG='-all',
               INCLUDE=windows(TC/'include'),LIB=windows(TC/'lib'),WINEPATH=windows(TC/'bin'))
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS'):env.pop(key,None)
    return env


if __name__=='__main__':
    if sys.argv[1]=='--init':validate_prefix();sys.exit(0)
    sys.exit(subprocess.call(['wine',windows(TC/'bin'/sys.argv[1])]+sys.argv[2:],env=environment()))
