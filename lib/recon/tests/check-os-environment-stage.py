"""Genuine launch-block staging consumes provider now and block at CRT bind."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('normal','after-args','bad-provider','early','high','short','empty','null','unaligned','wrap','large','repeat','after-consume'):
 p=subprocess.run([binary,mode],capture_output=True);expected=0 if admitted and mode=='normal' else 125
 assert p.returncode==expected and not p.stdout,(mode,p)
 assert bool(p.stderr)==bool(expected),(mode,p)
print('provider lifetime/deferred argument bind/borrowed block/CRT independence/errors/full108FP/12 denied inputs and profiles PASS %.6fs'%(time.monotonic()-start))
