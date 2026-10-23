import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
p=subprocess.run([binary],capture_output=True,check=True)
assert not p.stderr and p.stdout==bytes(4*528)
for mode in ('create','message','owner','flags','size','free','destroy','double','null','interior','wrongowner','overflow'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('private heap owner/full zeroed sizes/live payload/free/errors and 12 scope controls passed')
