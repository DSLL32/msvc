"""No originals. Test-owned initial payload proves fixed allocation does not zero."""
import pathlib
import subprocess
import sys
import time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
for mode in ('','live','flags','zero','owner','interior','twice'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if mode in ('','live') else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert b'Local' in p.stderr if expected else not p.stderr,(mode,p)
print('fixed malloc/nonzero test-owned contents; exact size/distinct ownership/free x3 errors xempty/live108FP;5 denials PASS %.6fs'%(time.monotonic()-start))
