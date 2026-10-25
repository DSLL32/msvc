"""Named module/export typing; checksum invocation and lifetime stay denied."""
import subprocess
import sys
import time

start = time.monotonic()
for mode in ('', 'live', 'preload', 'unknown', 'ordinal', 'null-name',
             'free', 'call', 'invalid', 'untouched'):
    result = subprocess.run([sys.argv[1]]+([mode] if mode else []), capture_output=True)
    expected = 0 if mode in ('', 'live', 'untouched') else 125
    assert result.returncode == expected and not result.stdout, (mode, result)
    if expected or mode == 'untouched':
        assert b'unsupported native operation:' in result.stderr, result.stderr
    else:
        assert not result.stderr, result.stderr
print('named IMAGEHLP load/export WINAPI4 typing, route errors/fullFP preserved; '
      'seven unsupported domains, no checksum access/write PASS %.6fs' %
      (time.monotonic()-start))
