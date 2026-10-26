import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr)
p=subprocess.run([binary,'other-size'],capture_output=True)
assert p.returncode==125,(p.returncode,p.stderr)
if len(sys.argv)>2:
    p=subprocess.run([str(Path(sys.argv[2]).resolve())],capture_output=True)
    assert p.returncode==125,(p.returncode,p.stderr)
if len(sys.argv)>3:
    p=subprocess.run([str(Path(sys.argv[3]).resolve())],capture_output=True)
    assert p.returncode==0 and not p.stderr,(p.returncode,p.stderr)
print('Watcom caller-frame400 and actual native remaining-stack controls pass')
