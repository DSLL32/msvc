"""Build the debugger with the original toolchain in its own Wine prefix."""
import fcntl
import hashlib
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
TC = ROOT/'toolchains/1.0-nt'
PREFIX = Path.home()/'.ai-tmp/vc1.0-fast-replay/wine'


def windows(path):
    return 'Z:'+str(Path(path).resolve()).replace('/', '\\')


def environment():
    env=os.environ.copy()
    env.update(WINEPREFIX=str(PREFIX),WINEARCH='win32',WINEDEBUG='-all',
               INCLUDE=windows(TC/'include'),LIB=windows(TC/'lib'),WINEPATH=windows(TC/'bin'))
    for key in ('CL','_CL_','LINK','_LINK_'):
        env.pop(key,None)
    return env


if __name__=='__main__':
    (HERE/'build').mkdir(exist_ok=True)
    with (HERE/'build/.lock').open('w') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        digest=hashlib.sha256(b''.join(p.read_bytes() for p in
            (HERE/'src/walker.c',Path(__file__),HERE/'Makefile'))).hexdigest()
        stamp=HERE/'build/source.sha256'
        if not ((HERE/'build/walker.exe').exists() and stamp.exists() and stamp.read_text()==digest):
            env=environment()
            if not (PREFIX/'system.reg').exists():
                PREFIX.parent.mkdir(parents=True,exist_ok=True)
                subprocess.run(['wineboot','-u'],env=env,check=True)
            subprocess.run(['wine',str(TC/'bin/CL.EXE'),'/nologo','/W3','/O1','/G3',
                            '/Fobuild/walker.obj','/Febuild/walker.exe','src/walker.c'],
                           cwd=HERE,env=env,check=True)
            stamp.write_text(digest)
