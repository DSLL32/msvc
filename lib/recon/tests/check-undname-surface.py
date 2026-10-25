"""Private MSVCRT entry typing only; no demangling or callbacks may execute."""
import re
import subprocess
import sys
import time

start = time.monotonic()
subprocess.run([sys.argv[1]], check=True)
for count in (1, 2, 3):
    result = subprocess.run([sys.argv[1]] + ['input'] * count, capture_output=True)
    assert result.returncode == 125 and not result.stdout, result
    assert b'unreviewed __unDName' in result.stderr, result.stderr
symbols = subprocess.check_output(['nm', '-u', sys.argv[1]], text=True)
assert not re.search(r'\b__unDName(?:@|$)', symbols, re.M), symbols
print('__unDName cdecl six-argument/callback typing; NULL/owned/unknown deny125, '
      'no host import or callback execution PASS %.6fs' % (time.monotonic()-start))
