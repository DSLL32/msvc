import subprocess,sys
binary=sys.argv[1]
result=subprocess.run([binary],capture_output=True)
assert result.returncode==0 and not result.stdout and not result.stderr,(result.returncode,result.stderr)
for mode in ('null','size','pointer','indirect','longjmp','generic-longjmp'):
    result=subprocess.run([binary,mode],capture_output=True)
    assert result.returncode==125,(mode,result.returncode,result.stderr)
print('real live-caller normalreturn0/52Bowner+guards/errors/full108Fx87 and6 scope denials passed')
