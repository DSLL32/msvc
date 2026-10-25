import subprocess
import sys
import time

start = time.monotonic()
subprocess.run([sys.argv[1]], check=True)
for args in (['null'], ['buffer', 'owned']):
    result = subprocess.run([sys.argv[1], *args], capture_output=True)
    assert result.returncode == 125, result
    assert not result.stdout
    assert b'unreviewed tmpnam' in result.stderr, result.stderr
print('tmpnam SDK cdecl surface: NULL and owned buffer deny125; %.6fs' %
      (time.monotonic() - start))
