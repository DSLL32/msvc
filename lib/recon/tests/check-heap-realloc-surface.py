"""SDK WINAPI four-argument boundary only; no reallocation semantics."""
import re
import subprocess
import sys
import time

start = time.monotonic()
subprocess.run([sys.argv[1]], check=True)
for count in (1, 2, 3):
    result = subprocess.run([sys.argv[1]]+['input']*count, capture_output=True)
    assert result.returncode == 125 and not result.stdout, result
    assert b'unreviewed HeapReAlloc' in result.stderr, result.stderr
symbols = subprocess.check_output(['nm', '-u', sys.argv[1]], text=True)
assert not re.search(r'\bHeapReAlloc(?:@|$)', symbols, re.M), symbols
print('HeapReAlloc WINAPI4 typing, NULL/owned/invalid deny125, no host import '
      'PASS %.6fs' % (time.monotonic()-start))
