"""SDK-typed boundary only; even count zero remains unsupported."""
import re
import subprocess
import sys
import time

start = time.monotonic()
subprocess.run([sys.argv[1]], check=True)
for count in (1, 2, 3):
    result = subprocess.run([sys.argv[1]] + ['input'] * count, capture_output=True)
    assert result.returncode == 125 and not result.stdout, result
    assert b'unreviewed _mbsnbcmp' in result.stderr, result.stderr
symbols = subprocess.check_output(['nm', '-u', sys.argv[1]], text=True)
assert not re.search(r'\b_mbsnbcmp(?:@|$)', symbols, re.M), symbols
print('_mbsnbcmp cdecl typed surface; NULL/count0, callerASCII, nonASCII deny125; '
      'no host import PASS %.6fs' % (time.monotonic()-start))
