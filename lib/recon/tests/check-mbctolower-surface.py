import subprocess
import sys

subprocess.run([sys.argv[1]], check=True)
for value in ('0', '65', '128', '255', '256', '0xffffffff'):
    result = subprocess.run([sys.argv[1], value], capture_output=True)
    assert result.returncode == 125, (value, result.returncode, result.stderr)
    assert b'unreviewed _mbctolower' in result.stderr
print('unsigned32 cdecl mapping surface compiles; six input scopes deny125')
