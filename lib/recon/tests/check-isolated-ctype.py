import subprocess
import sys
import hashlib
from pathlib import Path

binary, reference = sys.argv[1:3]
for name, expected in {
    'receipt.json': 'ebdbb204d8af623e8e00defcad03832006960fa7f4c88ec22317b76c0c638568',
    'locale-before-mbctype.bin': '2cdc20a27a8fc6b37dca7a733ffba7159c9a4feb9e07e4658a04e66def29e3ee',
    'locale-before-ctype.bin': '765bf4d1f9da8f6990d56bdbcfbe585290303b77e30cd481e7d99bca2a955725',
    'locale-after-ctype.bin': 'aba319e028b28540f24afb197c5735b0e29dc130fc8f5d165f27edf8de28296d',
}.items():
    assert hashlib.sha256((Path(reference) / name).read_bytes()).hexdigest() == expected, name
default = len(sys.argv) > 3 and sys.argv[3] == 'default'
modes = ('', 'live', 'unbound', 'provider', 'reference', 'reference-hex',
         'null-provider', 'null-reference', 'rebind', 'combined-first', 'combined-bound', 'ctype', 'mbtable',
         'pointer', 'mbcp', 'mbgate', 'mbmax', 'lccp', 'handle', 'category',
         'query', 'name', 'errno', 'doserrno', 'last', 'repeat', 'mb-api')
for mode in modes:
    result = subprocess.run([binary, reference, mode], capture_output=True)
    expected = 125 if default or mode not in ('', 'live') else 0
    assert result.returncode == expected, (mode, result.returncode, result.stdout, result.stderr)
    if expected == 0:
        assert not result.stderr and result.stdout == b'isolated CTYPE fulltables/old-new-result ownership/MB/errors/FP/inventory pass\n'
    else:
        assert b'recon: unsupported native operation:' in result.stderr
print('isolated CTYPE two fullstate positives/25 denials' if not default else 'default profile27 denied paths')
