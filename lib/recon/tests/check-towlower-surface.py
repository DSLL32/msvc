import subprocess
import sys
import time

start = time.monotonic()
subprocess.run([sys.argv[1]], check=True)
for count in range(1, 5):
    result = subprocess.run([sys.argv[1]] + ['input'] * count,
                            capture_output=True)
    assert result.returncode == 125, result
    assert not result.stdout
    assert b'unreviewed towlower' in result.stderr, result.stderr
print('towlower WCHAR16 cdecl surface: four code units deny125; %.6fs' %
      (time.monotonic() - start))
