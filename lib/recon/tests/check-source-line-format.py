import hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
record=json.loads((Path(__file__).parent/'source-line-format-original.json').read_text())
binary=str(Path(sys.argv[1]).resolve())
root=Path.home()/'.ai-tmp/msvc2/recon-maintenance/source-line-format'
root.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(dir=root) as tmp:
    p=subprocess.run([binary],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr,(p.returncode,p.stderr)
    assert len(p.stdout)==record['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==record['stdout_sha256']
    for name,digest in record['buffers'].items():
        assert hashlib.sha256((Path(tmp)/name).read_bytes()).hexdigest()==digest,name
    for mode in ('format','suffix','extra','locale','codepage','null'):
        p=subprocess.run([binary],cwd=tmp,env=dict(os.environ,RECON_SOURCE_LINE_NEGATIVE=mode),capture_output=True)
        assert p.returncode==125,(mode,p.returncode,p.stderr)
print('24 signed32/error rows,25 full8196-byte buffers and exact-space suffix; six unknown format/scope denials pass')
