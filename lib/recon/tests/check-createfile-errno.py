import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'createfile-errno-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'createfile-errno-reference.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    (Path(tmp)/'present.bin').write_bytes(b'payload')
    p=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stdout,p.stderr)
    assert (Path(tmp)/'present.bin').read_bytes()==b'payload'
    for i in range(2):
        for j in range(2):assert (Path(tmp)/('new-%d-%d.bin'%(i,j))).read_bytes()==b''
print('12 canonical mixed-case open/new/missing cases preserve independent CRT errno and actual LastError/status/file effects')
