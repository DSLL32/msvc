"""Native-only explicit510 semantics; original per-call state is unmeasured."""
import subprocess
import sys
import time

start = time.monotonic()
for mode in ('', 'live', 'null', 'nonascii', 'late-nonascii', 'handle',
             'codepage', 'mb-codepage', 'mb-width', 'partial'):
    result = subprocess.run([sys.argv[1]] + ([mode] if mode else []),
                            capture_output=True)
    expected = 0 if mode in ('', 'live', 'partial') else 125
    assert result.returncode == expected, (mode, result)
    assert not result.stdout
    if expected or mode == 'partial':
        assert b'_wcsupr' in result.stderr, result.stderr
    else:
        assert not result.stderr, result.stderr
print('131 spans x3 error tuples xempty/live108FP, alias/tails/guard pages/tables; '
      'seven denials and no partial write PASS %.6fs' % (time.monotonic()-start))
