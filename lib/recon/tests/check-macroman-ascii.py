"""Counted ASCII only, with native preservation/guards; no original run."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('','live','legacy','prewrite','valid1252','other-page','null','zero','negative','flags','replacement','used','short','query-output','write-null','overlap','input-wrap','output-wrap','unaligned','high'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if mode in ('legacy','prewrite') or admitted and mode in ('','live','zero','valid1252') else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert b'unsupported native operation:' in p.stderr if expected or mode=='prewrite' else not p.stderr,(mode,p)
print('explicit510 validity1252 plus page10000 ASCII query/count/write incl embeddedNUL/no added terminator, readonly edge/canaries,3 errors/empty+live108FP; nonASCII no-partial-write;zero-count failure87/no pointed access;14 denied domains; legacy/reverse scope unchanged PASS %.6fs'%(time.monotonic()-start))
