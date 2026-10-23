import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for mode in ('count','size','longjmp','acp','valid','next'):
    result=subprocess.run([binary,mode],capture_output=True)
    assert result.returncode==125,(mode,result.returncode,result.stderr)
print('caller-frame opaque context save and 6 fail-closed controls passed')
