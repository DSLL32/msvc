import subprocess,sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for mode in ['kernel','user','ntdll','ordinal','null']:
    p=subprocess.run([binary,mode],capture_output=True)
    assert p.returncode==125,(mode,p.returncode,p.stderr)
print('5knownexports/IsTNT absence preserve contracts;5unknown/ordinal controls deny')
