from pathlib import Path
import subprocess,sys
for path in sys.argv[1:]:
    p=subprocess.run([str(Path(path).resolve())],capture_output=True)
    assert p.returncode==0 and not p.stdout,(path,p.returncode,p.stdout,p.stderr)
    assert p.stderr.count(b'recon: unsupported native operation:')==6,(path,p.stderr)
print('PDB static eight-argument alias and implicit-operand no-formal floating import typing/no-access denials pass')
