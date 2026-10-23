import subprocess,sys
binary=sys.argv[1]
p=subprocess.run([binary],capture_output=True);assert p.returncode==0 and not p.stdout and not p.stderr,(p.returncode,p.stderr)
for mode in ('malloc','calloc','realloc','free','strdup','preactivity','unbound','repeat','rebind','bad-reference','null-reference'):
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('first-call empty target namespace/clean0→1/real delegation/errors/full x87 and11 denials passed')
