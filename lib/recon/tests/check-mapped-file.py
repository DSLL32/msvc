import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
reference=json.loads((Path(__file__).parent/'mapped-file-original.json').read_text())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(reference['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    assert hashlib.sha256((Path(tmp)/'mapped.dat').read_bytes()).hexdigest()==reference['artifact_sha256']
    for mode in ('high','address','length'):
        p=subprocess.run([binary,mode],cwd=tmp,capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('original mapped-size/whole-view-flush output+2MB file exact;3unsupported controls passed')
