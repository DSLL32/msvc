import hashlib,json,os,subprocess,sys
from pathlib import Path
v=json.loads((Path(__file__).parent/'wctype-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'wctype-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve());p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stderr and len(p.stdout)==v['stdout_bytes'] and hashlib.sha256(p.stdout).hexdigest()==v['stdout_sha256'],(p.returncode,p.stderr)
for mode in ('nonascii','mask','locale'):
    p=subprocess.run([binary],env=dict(os.environ,RECON_WCTYPE_NEGATIVE=mode),capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('all1024 original ASCII digit/space mask/error rows exact;3unsupported scopes denied')
