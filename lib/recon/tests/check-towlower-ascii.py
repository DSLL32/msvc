"""Native-only explicit510 semantics; original per-call state remains unmeasured."""
import subprocess
import sys
import time

start = time.monotonic()
for mode in ('', 'live', 'nonascii', 'high', 'locale', 'codepage',
             'mb-codepage', 'mb-width'):
    result = subprocess.run([sys.argv[1]] + ([mode] if mode else []),
                            capture_output=True)
    expected = 0 if mode in ('', 'live') else 125
    assert result.returncode == expected, (mode, result)
    assert not result.stdout
    if expected:
        assert b'towlower outside MSVCRT510 default-C ASCII scope' in result.stderr
    else:
        assert not result.stderr, result.stderr
print('128 ASCII x3 error tuples xempty/live108FP, locale tables unchanged; '
      'six scope denials PASS %.6fs' % (time.monotonic() - start))
