import hashlib,json,subprocess,sys
from pathlib import Path
v=json.loads((Path(__file__).parent/'getenv-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'getenv-reference.c').read_bytes()).hexdigest()==v['source_sha256']
p=subprocess.run([str(Path(sys.argv[1]).resolve())],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
print('canonical case-insensitive borrowed getenv pointer/mutation/missing-name/errors exact')
