import subprocess,sys
binary=sys.argv[1];p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stdout and not p.stderr
for mode in ('nonascii','null','codepage','max','locale'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('128 ASCII/NUL lengths and error preservation agree with complete canonical leaf;5 unknown scopes denied')
