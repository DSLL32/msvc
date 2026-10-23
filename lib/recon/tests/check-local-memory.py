import json,subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'local-memory-original.json').read_text())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
for mode in ['flags','owner','twice']:
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('original local allocation/free exact zeroed payload and errors,3ownership/scope controls')
