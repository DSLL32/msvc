import json,subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'mb-chr-original.json').read_text())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
for mode in ['codepage','locale','nonascii','value']:
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('canonical ASCII first-character/NUL/empty match,4scope denials')
