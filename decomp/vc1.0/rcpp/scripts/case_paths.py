"""Fixed shared Wine drives and isolated RCPP capture namespaces."""
from pathlib import Path
import fcntl
from contextlib import contextmanager

ROOT = Path(__file__).resolve().parents[4]
SCRATCH = Path.home()/'.ai-tmp/msvc2'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'
CAPTURES = SCRATCH/'vc1.0/rcpp'


def windows(path):
    path = Path(path).resolve()
    for drive, root in [('R:',ROOT),('S:',SCRATCH)]:
        try: relative = path.relative_to(root)
        except ValueError: continue
        return drive+'\\'+str(relative).replace('/','\\')
    raise ValueError('Capture/build path must be under fixed R: or S: roots: '+str(path))


_capture_depth = 0

@contextmanager
def capture_lock():
    global _capture_depth
    if _capture_depth:
        yield
        return
    CAPTURES.mkdir(parents=True,exist_ok=True)
    with (CAPTURES/'capture.lock').open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX)
        _capture_depth += 1
        try: yield
        finally: _capture_depth -= 1
