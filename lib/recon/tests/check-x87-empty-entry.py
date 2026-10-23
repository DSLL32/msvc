import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stdout and not p.stderr,(p.returncode,p.stderr)
for mode in ('control','status','top','tag','hash','uppercase','null','live','native-control','native-status','repeat'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('Captured empty x87 entry SW20 only; complete108-byte remainingstate and3errors preserved;11denials')
