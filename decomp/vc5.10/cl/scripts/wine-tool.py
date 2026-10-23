#!/usr/bin/env python3
"""Explicit original-toolchain build only; never imported by native fixtures."""
import os
from pathlib import Path
import subprocess
import sys
import time
import json

ROOT=Path(__file__).resolve().parents[4]
PROJECT=Path(__file__).resolve().parents[1]
PREFIX=Path('/root/.ai-tmp/vc1.0-capture/wine')
SCRATCH=Path('/root/.ai-tmp/msvc2')
TC=ROOT/'toolchains/5.10'


def windows(path):
    path=Path(path).resolve()
    for host,drive in ((ROOT,'R:'),(SCRATCH,'S:')):
        try:return drive+'\\'+str(path.relative_to(host)).replace('/','\\')
        except ValueError:pass
    raise ValueError('Path outside fixed R:/S: roots: '+str(path))


def environment():
    assert (PREFIX/'dosdevices/r:').resolve()==ROOT
    assert (PREFIX/'dosdevices/s:').resolve()==SCRATCH
    env=os.environ.copy()
    for key in ('CL','_CL_','LINK','_LINK_','MSC_CMD_FLAGS','_MSC_IDE_FLAGS'):
        env.pop(key,None)
    env.update(WINEPREFIX=str(PREFIX),WINEARCH='win32',WINEDEBUG='-all',
        WINEDLLOVERRIDES='msvcrt=n',INCLUDE=windows(TC/'include'),
        LIB=windows(TC/'lib'),WINEPATH=windows(TC/'bin'))
    return env


if __name__=='__main__':
    os.chdir(PROJECT)
    command=['/usr/bin/wine',windows(TC/'bin'/sys.argv[1]),*sys.argv[2:]]
    started=time.perf_counter()
    result=subprocess.run(command,env=environment(),timeout=60)
    elapsed=time.perf_counter()-started
    records=PROJECT/'build/timings';records.mkdir(parents=True,exist_ok=True)
    (records/('original-%d-%d.json'%(time.time_ns(),os.getpid()))).write_text(json.dumps({'command':command,'seconds':elapsed,'status':result.returncode},indent=2)+'\n')
    raise SystemExit(result.returncode)
