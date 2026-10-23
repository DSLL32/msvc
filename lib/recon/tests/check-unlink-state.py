import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'unlink-state-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'unlink-state-reference.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);(p/'guard.bin').write_bytes(bytes(range(256)))
    result=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=p,capture_output=True)
    assert result.returncode==0 and not result.stderr,(result.returncode,result.stderr)
    assert len(result.stdout)==v['stdout_bytes'] and hashlib.sha256(result.stdout).hexdigest()==v['stdout_sha256'],result.stdout
    assert (p/'guard.bin').read_bytes()==bytes(range(256))
    assert sorted(x.name for x in p.iterdir())==['blocked.bin','guard.bin']
    assert (p/'blocked.bin').read_bytes()==b'guard'
    (p/'directory').mkdir();(p/'symlink').symlink_to('guard.bin');os.mkfifo(p/'fifo')
    for argument in ('NULL','','directory','symlink','fifo','missing*'):
        denied=subprocess.run([str(Path(sys.argv[1]).resolve()),argument],cwd=p,capture_output=True)
        assert denied.returncode==125,(argument,denied.returncode,denied.stderr)
    assert (p/'guard.bin').read_bytes()==bytes(range(256))
    assert (p/'symlink').is_symlink() and (p/'directory').is_dir()

print('32 canonical unlink success/missing-leaf/parent error-state rows, input256-byte immutability and isolated file effects exact')
