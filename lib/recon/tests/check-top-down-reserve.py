import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stdout and not p.stderr,(p.returncode,p.stderr)
for mode in ('address','commit','top-only','unknown','protection','zero','short','unaligned','overflow'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('Two reached sizes/page boundary: owned64K-aligned private PROT_NONE spans, errors/live x87/release;9scope denials')
