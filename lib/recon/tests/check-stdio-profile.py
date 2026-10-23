import json,subprocess,sys,tempfile
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
v=json.loads((Path(__file__).parent/'stdio-profile-original.json').read_text())
with tempfile.TemporaryDirectory() as tmp:
    p=subprocess.run([binary,'data'],cwd=tmp,capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==bytes.fromhex(v['stdout_hex']),(p.returncode,p.stdout,p.stderr)
    assert (Path(tmp)/'stdio.dat').read_bytes()==bytes.fromhex(v['artifact_hex'])
print('canonical VC5 setvbuf owned/user/unbuffered/odd/invalid/flush transitions match')
