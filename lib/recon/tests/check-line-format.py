import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
v=json.loads((Path(__file__).parent/'line-format-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'line-format-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stderr)
    for name,digest in v['buffers'].items():assert hashlib.sha256((Path(tmp)/name).read_bytes()).hexdigest()==digest,name
    for mode in ('format','locale','codepage','null'):
        p=subprocess.run([binary],cwd=tmp,env=dict(os.environ,RECON_LINE_FORMAT_NEGATIVE=mode),capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('24 signed32/error rows and25 full8196-byte buffers exact;4 unsupported scopes denied')
