"""Bounded native contract; original per-call state remains unmeasured."""
import subprocess
import sys
import time

start = time.monotonic()
default = len(sys.argv) > 2 and sys.argv[2] == 'default'
for mode in ('', 'live', 'null-left', 'null-right', 'nonascii-left', 'nonascii-right',
             'locale', 'codepage', 'mb-codepage', 'mb-width'):
    result = subprocess.run([sys.argv[1]] + ([mode] if mode else []), capture_output=True)
    expected = 0 if mode in ('', 'live') and not default else 125
    assert result.returncode == expected and not result.stdout, (mode, result)
    if expected:
        assert b'unsupported native operation:' in result.stderr, result.stderr
    else:
        assert not result.stderr, result.stderr
print(('generic remains denied' if default else
       'ASCII fold/order/alias/prefix x3 errors xempty/live108FP; source/tables, '
       'guard-page stop and eight denials')+' PASS %.6fs' % (time.monotonic()-start))
