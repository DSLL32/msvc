import hashlib
import subprocess
import sys
from pathlib import Path

binary, directory = sys.argv[1:3]
reference = Path(directory)
receipt = hashlib.sha256((reference / 'receipt.json').read_bytes()).hexdigest()
assert receipt in ('d2dbc20e792aaba5b168a4c9c4c233ee306b407770d5be7527b5fdd204b857d5',
                   '224af79880cc992c95621c89c2ee807d6468f58685b8d3d8161871736cd1ec79')
for name, expected in {
    'locale-before-mbctype.bin': '2cdc20a27a8fc6b37dca7a733ffba7159c9a4feb9e07e4658a04e66def29e3ee',
    'locale-before-ctype.bin': '765bf4d1f9da8f6990d56bdbcfbe585290303b77e30cd481e7d99bca2a955725',
    'locale-after-ctype.bin': 'aba319e028b28540f24afb197c5735b0e29dc130fc8f5d165f27edf8de28296d',
}.items():
    assert hashlib.sha256((reference / name).read_bytes()).hexdigest() == expected
default = len(sys.argv) > 3 and sys.argv[3] == 'default'
modes = ('', 'live', 'unbound', 'combined', 'before-locale', 'argument', 'zero',
         'other-selector', 'errno', 'doserrno', 'last', 'mbcp', 'mbgate', 'mbmax',
         'lccp', 'handle', 'mbtable', 'ctype', 'oldtable', 'pointer', 'result',
         'repeat', 'locale-repeat')
for mode in modes:
    result = subprocess.run([binary, directory, receipt, mode], capture_output=True)
    expected = 125 if default or mode not in ('', 'live') else 0
    assert result.returncode == expected, (mode, result.returncode, result.stdout, result.stderr)
    if expected == 0:
        assert not result.stderr and result.stdout == b'LC_CTYPE Last0 then SBCS locale-codepage selection, fullstate/errors/FP/inventory pass\n'
    else:
        assert b'recon: unsupported native operation:' in result.stderr
print('LC_CTYPE Last0 +SBCS two positives/21 denials' if not default else 'default23 paths deny')
