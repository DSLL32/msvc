import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'wide-append-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'wide-append-reference.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    for name,digest in v['buffers'].items():assert hashlib.sha256((Path(tmp)/name).read_bytes()).hexdigest()==digest,name
print('actual1252 ASCII lead-byte results, UTF16 append alias/full8196-byte buffers/tails/errors exact across4 sentinel pairs')
