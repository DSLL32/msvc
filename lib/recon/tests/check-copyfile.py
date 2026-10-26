"""Run the CopyFileA probe in private native-only storage."""
import pathlib
import subprocess
import sys
import tempfile
import time

binary = pathlib.Path(sys.argv[1]).resolve()
base = pathlib.Path.home() / '.ai-tmp/msvc2/copyfile-check'
base.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='run-', dir=base) as directory:
    start = time.monotonic()
    result = subprocess.run([str(binary)], cwd=directory, capture_output=True, timeout=10)
    assert result.returncode == 0 and not result.stderr, (result.returncode, result.stdout, result.stderr)
    assert result.stdout.endswith(b'prewrite/denials PASS\n'), result.stdout
    print(result.stdout.decode().strip(), f'({time.monotonic() - start:.3f}s)')
