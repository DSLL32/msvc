import json,subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'module-memory-original.json').read_text())
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
for mode in ['hash','owner','range','size','heap','overlap','base','length','unregistered']:
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('image-backed36 byte flush original success/errors exact;9owner/range controls passed')
