"""Native-only focused checks; original capture bytes are read, never run."""
import hashlib,json,subprocess,sys
from pathlib import Path
binary,directory=sys.argv[1:3];d=Path(directory)
default=len(sys.argv)>3 and sys.argv[3]=='default'
c=json.loads((d/'capture.json').read_text())
assert hashlib.sha256((d/'capture.json').read_bytes()).hexdigest()=='79680de860f1333b121e5b9c5ffccaef614abdce0893d8c64ec536eb05f3adbb'
for name,record in c['files'].items():
 data=(d/name).read_bytes();assert len(data)==record['bytes'] and hashlib.sha256(data).hexdigest()==record['sha256'],name
before=d/'observer/lower-before-buffer.bin';after=d/'observer/lower-after-buffer.bin'
assert len(before.read_bytes())==len(after.read_bytes())==32
modes=['normal','live','null','nonascii','mbcp','mbgate','lccp','mbmax','handle','other-handle','mbtable','ctype','eof','pointer','bound','calls','mbcalls','errno','doserrno','last']
for mode in modes:
 r=subprocess.run([binary,str(before),str(after),mode],capture_output=True)
 expected=0 if not default and mode in ('normal','live') else 125
 assert r.returncode==expected,(mode,r.returncode,r.stdout,r.stderr)
 if expected:assert b'recon: unsupported native operation:' in r.stderr
 else:assert r.stdout==b'actual32B plus129 ASCII spans/alias/tails/errors/full108FP PASS\n' and not r.stderr
if not default:
 r=subprocess.run([binary,str(before),str(after),'partial'],capture_output=True)
 assert r.returncode==0 and r.stdout==b'prevalidation PASS\n' and b'unsupported' in r.stderr
print(('default20 denials' if default else 'actual32B/129 spans x empty/live FP +18 denials +no-partial-write')+' PASS')
