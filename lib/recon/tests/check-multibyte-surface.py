import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for name in ('compare','advance','copy','space','find'):
    subprocess.run([binary,name],check=True)
for name in ('upper','pbrk','substring'):
    result=subprocess.run([binary,name],capture_output=True)
    assert result.returncode==125,(name,result.returncode,result.stderr)
print('5 implemented ASCII imports compile/run;3 unvisited SDK imports explicitly unsupported')
