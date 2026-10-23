import subprocess
import sys
binary=sys.argv[1]
subprocess.run([binary],check=True)
for action in ('unbound','wildcard','rebind','fsopen'):
    result=subprocess.run([binary,action],capture_output=True)
    assert result.returncode==125,(action,result.returncode,result.stderr)
    assert b'unsupported native operation' in result.stderr
print('startup bindings/state and 4 fail-closed controls passed')
