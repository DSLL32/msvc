import hashlib,subprocess,sys
from pathlib import Path
p=subprocess.run([str(Path(sys.argv[1]).resolve())],capture_output=True)
assert p.returncode==0 and not p.stderr and hashlib.sha256(p.stdout).hexdigest()=='cc7b3ffdbbab9aa43c2c5a0167b82e4550eeeb1e8b7b6e05387b2c7c13e0e242',(p.returncode,p.stdout,p.stderr)
print('original mixed union8 return payload/RET4 transport; call and destination evaluated once')
