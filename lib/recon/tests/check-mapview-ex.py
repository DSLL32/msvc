import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'mapview-ex-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'mapview-ex-probe.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    binary=str(Path(sys.argv[1]).resolve());p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    assert hashlib.sha256((Path(tmp)/'mapped.dat').read_bytes()).hexdigest()==v['artifact_sha256']
    p=subprocess.run([binary,'fixed'],cwd=tmp,capture_output=True);assert p.returncode==125,(p.returncode,p.stderr)
print('canonical NULL-base MapViewOfFileEx/errors/full2MB zero extension/view lifetime/persistence exact; fixed denied')
