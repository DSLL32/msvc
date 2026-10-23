import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for name in ('compare','advance','copy'):
    subprocess.run([binary,name],check=True)
for name in ('space','find'):
    result=subprocess.run([binary,name],capture_output=True)
    assert result.returncode==125,(name,result.returncode,result.stderr)
print('5 SDK multibyte imports compile and fail explicitly when called')
