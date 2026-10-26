"""Cold SDK surface only; no directory/provider execution."""
import pathlib,subprocess,sys,time
start=time.monotonic();binary=str(pathlib.Path(sys.argv[1]).resolve())
for operation in ('open','read','close'):
    for argument in ('null','owned','invalid'):
        p=subprocess.run([binary,operation,argument],capture_output=True)
        assert p.returncode==125 and not p.stdout and b'unsupported native operation:' in p.stderr,(operation,argument,p)
print('packed289B SDK typing/offsets/attributes; profile aliases; three no-access boundaries xNULL/owned/invalid denied125 PASS %.6fs'%(time.monotonic()-start))
