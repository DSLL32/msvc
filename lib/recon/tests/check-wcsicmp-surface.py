"""WCHAR16 pointer ABI and unconditional denial; no host wchar32 conversion."""
import re
import subprocess
import sys
import time

start = time.monotonic()
subprocess.run([sys.argv[1]], check=True)
for count in (1, 2, 3, 4):
    result = subprocess.run([sys.argv[1]] + ['input'] * count, capture_output=True)
    assert result.returncode == 125 and not result.stdout, result
    assert b'unreviewed _wcsicmp' in result.stderr, result.stderr
symbols = subprocess.check_output(['nm', '-u', sys.argv[1]], text=True)
assert not re.search(r'\b_wcsicmp(?:@|$)', symbols, re.M), symbols
print('_wcsicmp WCHAR16 cdecl pointers; NULL/ASCII/empty/nonASCII deny125, '
      'no plain host import PASS %.6fs' % (time.monotonic()-start))
