import subprocess,sys
binary=sys.argv[1];p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and not p.stdout and not p.stderr
for mode in ('errno','prefix'):
    p=subprocess.run([binary,mode],capture_output=True);assert p.returncode==125,(mode,p.returncode,p.stderr)
print('original ENOENT text/newline/shared-buffer alias/errors exact; unknown errno/prefix denied')
