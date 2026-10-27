from pathlib import Path
import subprocess,sys
selected,generic,shim=[str(Path(p).resolve()) for p in sys.argv[1:]]
for binary,args,expected in [(selected,[],0),(generic,[],125),(shim,[],0)]+[(selected,[str(n)],125) for n in (0,2,-1,15,16,-2147483648,2147483647)]:
    p=subprocess.run([binary]+args,capture_output=True)
    assert p.returncode==expected and not p.stdout,(binary,args,p.returncode,p.stdout,p.stderr)
    if expected: assert b'recon: unsupported native operation:' in p.stderr
    else: assert not p.stderr
print('Watcom errno1 exact stable text/typing/errors/semanticFP, othernumber/profile denials and host isolation pass')
