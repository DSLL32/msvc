from pathlib import Path
import subprocess, sys, tempfile
preferred, generic = [str(Path(p).resolve()) for p in sys.argv[1:]]
with tempfile.TemporaryDirectory(prefix='preferred-view-', dir=Path.home()/'.ai-tmp') as tmp:
    run = subprocess.run([preferred], cwd=tmp, capture_output=True)
    assert run.returncode == 0 and not run.stderr, (run.returncode, run.stdout, run.stderr)
    data = (Path(tmp)/'view.dat').read_bytes()
    assert len(data) == 0x40000 and data[0] == 0x12 and data[-1] == 0x34 and not any(data[1:-1])
    for binary, mode in [(generic, ''), (preferred, 'short'), (preferred, 'access'), (preferred, 'offset')]:
        run = subprocess.run([binary] + ([mode] if mode else []), cwd=tmp, capture_output=True)
        assert run.returncode == 125, (binary, mode, run.returncode, run.stderr)
print('preferred RW success/collision/alignment/lifetime/state and finite domain isolation pass')
