import hashlib,json,os,subprocess,sys
from pathlib import Path
v=json.loads((Path(__file__).parent/'cpinfo-original.json').read_text())
assert hashlib.sha256((Path(__file__).parent/'cpinfo-reference.c').read_bytes()).hexdigest()==v['source_sha256']
binary=str(Path(sys.argv[1]).resolve());expected=bytes.fromhex(v['stdout_hex'])
for alternate in (False,True):
    env=dict(os.environ,RECON_ACP='1252')
    if alternate:env['RECON_CPINFO_PADDING']='1'
    p=subprocess.run([binary],env=env,capture_output=True)
    oracle=expected.replace(b'5a5a\r\n',b'a7c9\r\n') if alternate else expected
    assert p.returncode==0 and not p.stderr and p.stdout==oracle,(alternate,p.returncode,p.stdout,p.stderr)
print('actual ACP1252 GetCPInfo fields/errors exact; native padding preserved independently')
