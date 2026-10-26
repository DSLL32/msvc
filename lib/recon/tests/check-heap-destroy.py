"""Release only a tracked heap and every outstanding mapping it owns."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve());generic=len(sys.argv)>2 and sys.argv[2]=='default'
for mode in ('','live','null','unknown','interior','dead','dead-alloc'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if not generic and mode in ('','live') else 125
    assert p.returncode==expected and not p.stdout,(mode,p.returncode,p.stderr)
    assert bool(p.stderr)==bool(expected),(mode,p.stderr)
print('tracked heap/block retirement/independent owners/empty owner/errors/full108FP;5 denials PASS %.6fs'%(time.monotonic()-start))
