import hashlib,json,subprocess,sys
from pathlib import Path
binary,directory=sys.argv[1:3];reference=Path(directory)
receipt=hashlib.sha256((reference/'receipt.json').read_bytes()).hexdigest()
references={
 '8be7252b2181a2133ca2a8d161e30b1aec235f686f09671080170d7d8df77322':256,
 '73ded0bc972d05a4672481f86328a37a3116ae51b59134c8a270c21f90c09f52':255,
 '407c8063150937f6fb78aa13ddea9bbfa3059843b04782a5218b618bd2480efd':256,
 'efa1b836f576e9d876c4024ace9376194d4e4b131fc3dcb1aeaf2ed6d6efbb85':255}
count=references[receipt]
# Original-only semantic validators are independently run before freezing this
# native probe. Recheck the pinned receipt's complete artifact closure here.
for name,record in json.loads((reference/'receipt.json').read_text())['artifacts'].items():
 data=(reference/name).read_bytes()
 assert len(data)==record['size'] and hashlib.sha256(data).hexdigest()==record['sha256'],name
default=len(sys.argv)>3 and sys.argv[3]=='default'
modes=('', 'live','sentinel','unbound','combined','before-locale','before-mb',
       'wide','uintmax','bound','calls','mbcalls','mbcp','mbgate','mbmax','lccp','handle','other-handle',
       'mbtable','ctype','oldtable','pointer','result')
for mode in modes:
 r=subprocess.run([binary,directory,receipt,mode,str(count)],capture_output=True)
 expected=125 if default or mode not in ('','live','sentinel') else 0
 assert r.returncode==expected,(mode,r.returncode,r.stdout,r.stderr)
 if expected==0:
  assert r.stdout==b'actual byte-domain lowercase vector, tables/owners/errors/fullFP preserved\n' and not r.stderr
 else:assert b'recon: unsupported native operation:'in r.stderr
print(('default23 denials'if default else str(count)+' actual rows x empty/live/sentinel +20 denials')+' PASS')
