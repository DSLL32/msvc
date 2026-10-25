"""Actual lowio mode mutation; no originals or recorded API results."""
import pathlib
import subprocess
import sys
import tempfile
import time
start=time.monotonic()
binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
with tempfile.TemporaryDirectory() as cwd:
    path=pathlib.Path(cwd,'binary.dat');path.write_bytes(b'real binary input\r\n\x1a')
    for mode in ('','live','invalid','bound','text','ms','zero','closed'):
        result=subprocess.run([binary]+([mode] if mode else []),cwd=cwd,capture_output=True)
        expected=0 if mode in ('','live') and not generic else 125
        assert result.returncode==expected and not result.stdout,(mode,result)
        assert (b'Watcom' in result.stderr) if expected else not result.stderr,(mode,result)
        assert path.read_bytes()==b'real binary input\r\n\x1a'
print(('generic denied' if generic else 'binary transition/old-mode translation on stdin/stdout/regular fd, exact flags/handles/FILE x3 errors xempty/live108FP;6 denials')+' PASS %.6fs'%(time.monotonic()-start))
