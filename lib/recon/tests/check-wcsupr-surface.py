"""Typed WCHAR16 pointer link surface only; no original execution."""
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
    assert b'unreviewed _wcsupr' in result.stderr, result.stderr
print('_wcsupr WCHAR16* cdecl surface: NULL/empty/ASCII/nonASCII deny125; %.6fs' %
      (time.monotonic() - start))
