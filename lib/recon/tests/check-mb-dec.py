import json,subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'mb-dec-original.json').read_text())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
for mode in ['codepage','locale','nonascii']:
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('canonical ASCII backwards pointer/bounds/NUL exact;3scope controls passed')
