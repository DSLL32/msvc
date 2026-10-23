import hashlib,json,os,subprocess,sys
from pathlib import Path
v=json.loads((Path(__file__).parent/'ascii-terminal-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'ascii-terminal-reference.c').read_bytes()).hexdigest()==v['source_sha256']
env=dict(os.environ,RECON_ACP='1252')
p=subprocess.run([str(Path(sys.argv[1]).resolve())],env=env,stdin=subprocess.DEVNULL,capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
env['RECON_NONASCII_CONTROL']='1'
p=subprocess.run([str(Path(sys.argv[1]).resolve())],env=env,stdin=subprocess.DEVNULL,capture_output=True)
assert p.returncode==125,(p.returncode,p.stderr)
print('canonical isatty FDEV64/zero/bounds and ASCII CharNext advance/NUL/errors exact; high byte denied')
