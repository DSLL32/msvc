import hashlib,json,subprocess,sys
from pathlib import Path
reference=Path(__file__).parent/'locale-profile-reference';record=json.loads((reference/'reference.json').read_text())
for name,digest in record['files'].items():assert hashlib.sha256((reference/name).read_bytes()).hexdigest()==digest,name
binary=str(Path(sys.argv[1]).resolve())
p=subprocess.run([binary,str(reference)],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==b'all257 mb bytes/257 ctype words/scalars/pointer ownership/errors exact\n',(p.returncode,p.stdout,p.stderr)
negative=('provider','kind','mb-count','ctype-count','mb','ctype-ascii','ctype-high','ctype-eof','initial-ctype','initial-mb','initial-codepage','initial-handle','repeat','unproved-api')
for mode in negative:
    p=subprocess.run([binary,str(reference),mode],capture_output=True)
    assert p.returncode==125 and b'unsupported' in p.stderr,(mode,p.returncode,p.stderr)
if len(sys.argv)>2:
    p=subprocess.run([str(Path(sys.argv[2]).resolve()),str(reference),'default'],capture_output=True)
    assert p.returncode==125 and b'without explicit MSVCRT510 profile' in p.stderr,(p.returncode,p.stderr)
print('all encountered locale table entries/EOF/static-vs-allocated identities/errors and14 profile/API denials pass; default remains closed')
