import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'fstat-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'fstat-probe.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'input.bin';path.write_bytes(bytes.fromhex(v['input_hex']));os.utime(path,(v['atime'],v['mtime']))
    p=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=tmp,stdin=subprocess.DEVNULL,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
print('canonical36-byte fstat disk/char/pipe/bounds/closed fields, untouched padding and error state exact')
