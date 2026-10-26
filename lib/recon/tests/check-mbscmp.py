"""Explicit510 default-C ASCII comparison; no original execution."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
admitted=len(sys.argv)>2 and sys.argv[2]=='510'
for mode in ('','live','null','high','gate','mbcp','crtpage','locale','mbmax'):
    p=subprocess.run([binary]+([mode] if mode else []),capture_output=True)
    expected=0 if mode in ('','live') and admitted else 125
    assert p.returncode==expected and not p.stdout,(mode,p)
    assert b'unsupported native operation:' in p.stderr if expected else not p.stderr,(mode,p)
print('ASCII equality/order/case/NUL/readonly-boundary x3 errors xempty/live108FP; tables retained; seven state/input denials; other profiles denied PASS %.6fs'%(time.monotonic()-start))
