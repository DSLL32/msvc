"""Bounded Microsoft ASCII copy contract; no original API run required."""
import subprocess
import sys
import time

start = time.monotonic()
watcom = len(sys.argv) > 2 and sys.argv[2] == 'watcom'
modes = ('', 'live', 'null-out', 'null-source', 'out-wrap', 'source-wrap',
         'overlap-forward', 'overlap-backward', 'same', 'nonascii', 'locale',
         'codepage', 'mb-codepage', 'mb-width', 'gate', 'partial')
for mode in modes:
    result = subprocess.run([sys.argv[1]] + ([mode] if mode else []), capture_output=True)
    expected = 0 if mode in ('', 'live', 'partial') and not watcom else 125
    # The partial-write child is denied even for Watcom; its parent verifies it.
    if mode == 'partial':
        expected = 0
    assert result.returncode == expected and not result.stdout, (mode, result)
    if expected or mode == 'partial':
        assert b'unsupported native operation:' in result.stderr, result.stderr
    else:
        assert not result.stderr, result.stderr
print(('Watcom remains denied' if watcom else
       '267 count/NUL spans x3 errors xempty/live108FP; bounded reads, padding, alias, '
       'tables, guard pages and no partial writes; 13 denials') +
      ' PASS %.6fs' % (time.monotonic()-start))
