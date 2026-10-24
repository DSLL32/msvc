from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
for mode in ('', 'live'):
    with tempfile.TemporaryDirectory() as directory:
        result = subprocess.run([binary, mode], cwd=directory, capture_output=True)
        assert result.returncode == 0, (mode, result.returncode, result.stdout, result.stderr)
        assert not result.stderr and result.stdout == b'mapped failure then successful NULL-high seek preserves1224; old highword/failures/FP/file effects pass\n'
        assert sorted(p.name for p in Path(directory).iterdir()) == ['seek.dat']
        assert (Path(directory) / 'seek.dat').read_bytes() == bytes(32) + b'seek' + bytes(8192 - 36)
print('actual three-call sequence/errors/position/full8192Bfile/fullFP and unchanged highword/failure controls pass')
