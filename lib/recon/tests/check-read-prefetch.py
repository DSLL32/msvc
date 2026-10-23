import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'read-prefetch-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'read-prefetch-probe.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    (Path(tmp)/'input.bin').write_bytes(b'a'*8192)
    p=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
print('canonical VC5 seek/refill512→4096 FILE/physical/logical positions; explicit buffer exclusion exact')
