import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'tempnam-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'tempnam-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    drive=Path(tmp);directory=drive/'recon-maintenance/tempnam-original/tmp';directory.mkdir(parents=True)
    env=dict(os.environ,RECON_DRIVE_S=tmp,RECON_TEMPNAM_PROFILE='libc100')
    p=subprocess.run([binary,v['actual_TMP']],cwd=drive,env=env,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stdout,p.stderr)
    assert not list(directory.iterdir())
    for mode in ('missing','dir','prefix','counter','basename','locale','relative','collision','profile','file'):
        changed=env.copy();argument=v['actual_TMP']
        if mode=='relative':argument='relative'
        if mode=='profile':changed['RECON_TEMPNAM_PROFILE']='unknown'
        if mode=='collision':(directory/'tmp2').write_bytes(b'collision')
        if mode=='file':directory.rmdir();directory.write_bytes(b'file')
        p=subprocess.run([binary,argument,mode],cwd=drive,env=changed,capture_output=True)
        assert p.returncode==125 and b'unsupported' in p.stderr,(mode,p.returncode,p.stderr)
        if mode=='collision':assert (directory/'tmp2').read_bytes()==b'collision';(directory/'tmp2').unlink()
        if mode=='file':assert directory.read_bytes()==b'file';directory.unlink();directory.mkdir()
print('4 libc100 tempnam counter/TMP/error triples/owned results exact; no creation;10 scope denials pass')
