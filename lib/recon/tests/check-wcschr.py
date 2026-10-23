import subprocess,sys
binary=sys.argv[1]
p=subprocess.run([binary],capture_output=True)
assert p.returncode==0 and p.stdout==b'28 canonical rows and six code-unit/termination controls passed\n' and not p.stderr,(p.returncode,p.stdout,p.stderr)
p=subprocess.run([binary,'null'],capture_output=True)
assert p.returncode==125,(p.returncode,p.stdout,p.stderr)
print('UTF16 wcschr: first/absent/repeated/high/zero, full inputs/errors/live108FP and NULL denial pass')
