import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'fullpath-errno-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'fullpath-errno-reference.c').read_bytes()).hexdigest()==v['source_sha256']
with tempfile.TemporaryDirectory() as tmp:
    work=Path(tmp)/'recon-maintenance/fullpath-errno-original';work.mkdir(parents=True)
    env=dict(os.environ,RECON_DRIVE_S=tmp,RECON_CANONICAL_CWD='S:\\recon-maintenance\\fullpath-errno-original')
    p=subprocess.run([str(Path(sys.argv[1]).resolve())],cwd=work,env=env,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stdout,p.stderr)
    for name,digest in v['buffers'].items():assert hashlib.sha256((work/name).read_bytes()).hexdigest()==digest,name
print('8 canonical successful fullpath aliases/errno/LastError/complete260-byte buffers and untouchedtails exact')
