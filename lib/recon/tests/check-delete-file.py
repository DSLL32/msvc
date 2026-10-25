"""Native regular closed-file deletion and explicit unsupported ownership."""
import pathlib
import subprocess
import sys
import tempfile
import time

start = time.monotonic()
binary = str(pathlib.Path(sys.argv[1]).resolve())
for mode in ('', 'live', 'null', 'empty', 'wildcard', 'directory', 'symlink',
             'open', 'hardlink', 'mapping', 'view'):
    with tempfile.TemporaryDirectory() as directory:
        result = subprocess.run([binary] + ([mode] if mode else []), cwd=directory,
                                capture_output=True)
        expected = 0 if mode in ('', 'live') else 125
        assert result.returncode == expected and not result.stdout, (mode, result)
        if expected:
            assert b'DeleteFileA' in result.stderr, result.stderr
            victim = pathlib.Path(directory, 'victim.bin')
            assert victim.stat().st_size == 2097152
            with victim.open('rb') as stream:
                assert stream.read(13) == b'owned payload'
        else:
            assert not result.stderr and not list(pathlib.Path(directory).iterdir()), result
print('real2MiB deletion/errors x3 tuples xempty/live108FP; FILE/lowio invariance; '
      'same-inode view/fd denial, other-inode live-view success, nine denials '
      'PASS %.6fs' % (time.monotonic()-start))
