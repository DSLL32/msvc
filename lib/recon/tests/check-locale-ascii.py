import hashlib,json,subprocess,sys,time
from pathlib import Path
tests=Path(__file__).resolve().parent
record=json.loads((tests/'locale-ascii-original.json').read_text())
binary=str(Path(sys.argv[1]).resolve())
reference=str(tests/'locale-profile-reference')
started=time.monotonic()
p=subprocess.run([binary,reference],capture_output=True)
assert p.returncode==0 and not p.stderr,(p.returncode,p.stderr)
assert hashlib.sha256(p.stdout).hexdigest()==record['stdout_sha256'],'original68624-row fingerprint mismatch'
assert len(p.stdout.splitlines())-1==record['rows']
negative=('no-profile','codepage','lc-codepage','handle','ctype','mb-table','max','pointer','mb-dec','mb-icmp','mb-nbicmp','mb-pbrk','wcstol','searchenv','nonascii-inc','nonascii-clen','nonascii-space','nonascii-copy','nonascii-compare','nonascii-chr','nonascii-span','nonascii-getenv','nonascii-value')
for mode in negative:
    p=subprocess.run([binary,reference,mode],capture_output=True)
    assert p.returncode==125 and b'unsupported' in p.stderr,(mode,p.returncode,p.stderr)
print(f"exact68624 original ASCII/profile/error/alias rows and{len(negative)} unsupported API/profile/data denials pass ({time.monotonic()-started:.3f}s)")
