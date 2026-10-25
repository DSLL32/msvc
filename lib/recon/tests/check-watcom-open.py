"""Real binary descriptor operations; no original or recorded API result."""
import pathlib
import subprocess
import sys
import tempfile
import time

start = time.monotonic()
binary = str(pathlib.Path(sys.argv[1]).resolve())
generic = len(sys.argv) > 2 and sys.argv[2] == 'default'
payload = bytes(range(256))*4
for mode in ('', 'live', 'mode', 'null', 'empty', 'wildcard', 'nonascii',
             'directory', 'symlink', 'missing', 'missing-parent', 'missing-notdir', 'failure', 'seek-invalid',
             'seek-origin', 'seek-negative', 'seek-closed', 'tell-invalid', 'tell-closed'):
    with tempfile.TemporaryDirectory() as directory:
        path = pathlib.Path(directory, 'binary.dat')
        path.write_bytes(payload)
        path.chmod(0o444)
        pathlib.Path(directory, 'alias.dat').symlink_to('binary.dat')
        result = subprocess.run([binary]+([mode] if mode else []), cwd=directory,
                                capture_output=True)
        expected = 0 if mode in ('', 'live', 'missing', 'missing-parent', 'missing-notdir') and not generic else 125
        assert result.returncode == expected and not result.stdout, (mode, result)
        if expected:
            assert b'Watcom' in result.stderr, result.stderr
        else:
            assert not result.stderr, result.stderr
        assert path.read_bytes() == payload
print(('generic remains denied' if generic else
       'readonly binary fd/position/raw bytes/seek/tell/close; x3 errors xempty/live108FP, '
       'FILE unchanged; privateerrno missing leaf/parent, fourteen unsupported/failure denials')+' PASS %.6fs' % (time.monotonic()-start))
