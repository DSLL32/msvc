import subprocess,sys
binary=sys.argv[1];p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stdout and not p.stderr
for mode in ('text','set','null','codepage','locale'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('actual hello.obj wildcard absence, first-match pointer identity, empty/duplicate-set/error semantics and5 scope denials passed')
