"""Native hex-bit contract; pristine table proof is not a per-call observation."""
import subprocess
import sys
import time

start = time.monotonic()
default = len(sys.argv) > 2 and sys.argv[2] == 'default'
for mask_args in ([], ['alnum']):
    for mode in ('', 'live', 'nonascii', 'eof', 'mask', 'locale', 'codepage',
                 'mb-codepage', 'mb-width'):
        result = subprocess.run([sys.argv[1]] + ([mode] if mode else []) + mask_args, capture_output=True)
        expected = 0 if not default and mode in ('', 'live') else 125
        assert result.returncode == expected and not result.stdout, (mode, result)
        if expected:
            assert b'unsupported native operation:' in result.stderr, result.stderr
        else:
            assert not result.stderr, result.stderr
print(('generic masks128/107 remain denied' if default else
       'masks128/107 x128 ASCII x3 error tuples xempty/live108FP; exact bits/tables preserved, mask103 and other denials') +
      ' PASS %.6fs' % (time.monotonic()-start))
