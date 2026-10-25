"""Registration bookkeeping only; never deliver a signal or run an original."""
import pathlib
import subprocess
import sys
import time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
generic=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ('','live','fpe','ill','negative','zero','large','null','err','raise'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if mode in ('','live') and not generic else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert b'Watcom' in p.stderr if expected else not p.stderr,(mode,p)
print(('generic denied' if generic else 'initial tokens/old-store/independent slots/callback identity x3 errors xempty/live108FP;host dispositions unchanged;8 denials')+' PASS %.6fs'%(time.monotonic()-start))
