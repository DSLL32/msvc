import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
for mode in ('','ordinal'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==0 and not p.stderr and p.stdout==b'module identity/typed callbacks/exports/single lease/errors exact\r\n',(mode,p.returncode,p.stdout,p.stderr)
negative=('hash','duplicate-name','duplicate-ordinal','original-address','owner','registration','early-lookup','early-getmodule','early-free','lifecycle-errors','path','repeat-load','missing-lookup','unknown-name','unknown-ordinal','repeat-lookup','foreign-free','repeat-free','reload','late-lookup','late-getmodule','cached')
for mode in negative:
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125 and b'unsupported' in p.stderr,(mode,p.returncode,p.stderr)
print('typed name/ordinal callbacks/single module lease/errors and22 identity/state denials pass')
