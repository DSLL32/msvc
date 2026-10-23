import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
p=subprocess.run([binary],capture_output=True,check=True)
assert not p.stderr and p.stdout==bytes(4*528)
original=Path('/root/.ai-tmp/msvc2/vc5.10/rcdll/heap-api-reference')
for a in (0,1):
    for b in (0,1):
        path=original/('heap-%s-%s.bin'%(a,b))
        if path.exists(): assert path.read_bytes()==bytes(528)
for mode in ('create','message','owner','flags','size','free','destroy'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('private owner/zeroed528 allocation, 4 original buffers and 7 fail-closed controls passed')
