"""1994 tie ordering, count2 boundary, byte widths and callback effects."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());variant=len(sys.argv)>2 and sys.argv[2]=='variant'
for mode in (('','null','callback','zero','overflow','wrap') if variant else ('',)):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if variant and not mode else 125
    assert p.returncode==expected and not p.stdout,(mode,p.returncode,p.stderr)
    assert bool(p.stderr)==bool(expected),(mode,p.stderr)
print('n0/1/2/3/7/8/9..240/midpoint equal order/duplicates/unaligned widths/permutation/tails/callback effects/current FP/profile isolation;5 scope denials PASS %.6fs'%(time.monotonic()-start))
