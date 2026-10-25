"""Guarded ASCII prefix contract; no originals or broad table campaign."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
watcom=len(sys.argv)>2 and sys.argv[2]=='watcom'
for mode in ('','live','null','high','gate','mbcp','crtpage','locale','mbmax'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if mode in ('','live') and not watcom else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert b'unsupported native operation:' in p.stderr if expected else not p.stderr,(mode,p)
print('18 prefix/count/alias/guard-page cases x3 errors xempty/live108FP;tables/readonly sources;7 denials/Watcom denied PASS %.6fs'%(time.monotonic()-start))
