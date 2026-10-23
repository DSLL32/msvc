import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'writefile-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'writefile-probe.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    assert (Path(tmp)/'written.bin').read_bytes()==bytes.fromhex(v['artifact_hex'])
print('canonical successful regular-file WriteFile bytes/count/errno/LastError exact')
