import os,subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
original=(Path(__file__).parent/'mb-ascii-original.stdout').read_bytes()
p=subprocess.run([binary],capture_output=True,check=True)
assert not p.stderr and p.stdout==original,(p.stdout,original)
for mode in ('codepage','locale','nonascii-inc','nonascii-copy','nonascii-compare'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('canonical C0 ASCII compare/advance/copy exact original output and5scope controls passed')
