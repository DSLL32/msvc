import hashlib,json,subprocess,sys
from pathlib import Path
v=json.loads((Path(__file__).parent/'mb-icmp-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'mb-icmp-probe.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve());p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
for mode in ('codepage','locale','nonascii'):
    p=subprocess.run([binary,mode],capture_output=True);assert p.returncode==125,(mode,p.returncode,p.stderr)
print('canonical ASCII _mbsicmp folding/prefix/NUL/normalized results/errors exact;3scope denials')
