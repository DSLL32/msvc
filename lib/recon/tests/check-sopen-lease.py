import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve());packet=Path(sys.argv[2]);reference=json.loads(packet.read_text())
assert hashlib.sha256(packet.read_bytes()).hexdigest()=='87bb815753fcd92f716ec154b0cbff03ab7735d7f0d1f30d443e1d813ecbb67c'
raw=packet.parent/'analysis/observed-prefix-5/original';copy=reference['intermediate_files'][0];source=raw/copy['owned_contents'];contents=source.read_bytes()
assert len(contents)==25600 and hashlib.sha256(contents).hexdigest()==copy['sha256']=='4c97424c8baed3e1261bce5f95816ba01a995f752c1e9887466ddec58266cb15'
default=len(sys.argv)>3 and sys.argv[3]=='default'
modes=('', 'live','null','flags','access','sharing','permission','existing','dangling','directory','prior-mapping','alias','repeat','dup','dup2','mapping','raw-alias')
for mode in modes:
 with tempfile.TemporaryDirectory()as tmp:
  r=subprocess.run([binary,str(source.resolve()),mode],cwd=tmp,capture_output=True)
  expected=125 if default or mode not in ('','live')else 0
  assert r.returncode==expected,(mode,r.returncode,r.stdout,r.stderr)
  if expected==0:
   assert r.stdout==b'genuine exclusive binary noinherit lease/file/position/errors/FP/ownership pass\n'and not r.stderr
   assert (Path(tmp)/'lease.idb').read_bytes()==contents
  else:assert b'recon: unsupported native operation:'in r.stderr
print('default17denials PASS'if default else'genuine actual25600B lease/empty+live108FP/errors/stdio/descriptor aliases +15denials PASS')
