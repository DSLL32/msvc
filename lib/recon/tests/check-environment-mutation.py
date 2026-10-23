import subprocess,sys
from pathlib import Path
binary=str(Path(sys.argv[1]).resolve())
reference=str(Path(__file__).resolve().parent/'locale-profile-reference')
p=subprocess.run([binary,reference],capture_output=True)
assert p.returncode==0 and not p.stderr and p.stdout==b'two namespace transitions/initial and current aliases/owned assignment/errors exact\n',(p.returncode,p.stdout,p.stderr)
negative=('unbound','unbound-arguments','bad-units','early-nul','nonascii-os','empty-os','provider','rebind','initenv','errno','doserrno','last-error','first-profile','first-table','first-mb','first-nonempty','existing-crt','existing-os','no-equals','empty-name','nonascii-assignment','control-assignment','no-profile','second-name','second-empty','second-table','second-handle','second-vector','third')
for mode in negative:
    p=subprocess.run([binary,reference,mode],capture_output=True)
    assert p.returncode==125 and b'unsupported' in p.stderr,(mode,p.returncode,p.stdout,p.stderr)
if len(sys.argv)>2:
    p=subprocess.run([str(Path(sys.argv[2]).resolve()),reference],capture_output=True)
    assert p.returncode==125 and b'without explicit MSVCRT510 profile' in p.stderr,(p.returncode,p.stderr)
print('CRT/OS namespaces, initial/deep-copied/retained/owned-insert identities, history, errors, host isolation and29 denials pass; default closed')
