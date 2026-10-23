import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'access-state-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'access-state-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    work=Path(tmp);(work/'present.bin').write_bytes(bytes.fromhex(v['input_hex']))
    p=subprocess.run([binary],cwd=work,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stdout,p.stderr)
    assert sorted(x.name for x in work.iterdir())==['present.bin']
    assert (work/'present.bin').read_bytes()==bytes.fromhex(v['input_hex'])
    (work/'loop').symlink_to('loop');os.mkfifo(work/'fifo')
    for name in ('loop','fifo'):
        p=subprocess.run([binary,name],cwd=work,capture_output=True)
        assert p.returncode==125 and b'unsupported' in p.stderr,(name,p.returncode,p.stderr)
print('24 canonical mode0/4 result/error triples and unchanged file effects; unknown failure/file-kind denials pass')
