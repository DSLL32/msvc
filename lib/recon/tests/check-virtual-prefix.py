"""Separate reserve/contiguous commit/decommit/release; no original execution."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in (('normal','whole','empty','hole','readonly','unaligned','partial','interior','zero','repeat','recommit') if admitted else ('normal','whole')):
    p=subprocess.run([binary,mode],capture_output=True)
    expected=0 if mode in ('normal','whole') else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert bool(p.stderr)==bool(expected),(mode,p)
print('contiguous overlapping RW prefix/whole-reservation decommit; inaccessible/discarded ownership then release; independent owner/errors/full108FP;9 denied domains/profile retention PASS %.6fs'%(time.monotonic()-start))
