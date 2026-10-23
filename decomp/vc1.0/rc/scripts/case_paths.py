"""Fixed original-reference namespace; native cases use isolated S: mirrors."""
from contextlib import contextmanager
import fcntl
from pathlib import Path

ROOT=Path(__file__).resolve().parents[4]
SCRATCH=Path.home()/'.ai-tmp/msvc2'
PREFIX=Path.home()/'.ai-tmp/vc1.0-capture/wine'
CAPTURES=SCRATCH/'vc1.0/rc'


def windows(path):
    # Preserve inert child aliases: resolving a symlink changes the PATH spelling.
    path=Path(path).absolute()
    for drive,root in [('R:',ROOT),('S:',SCRATCH)]:
        try:relative=path.relative_to(root)
        except ValueError:continue
        return drive+'\\'+str(relative).replace('/','\\')
    raise ValueError('Reference paths must be under fixed R: or S: roots: '+str(path))


@contextmanager
def capture_lock(name):
    assert name and '/' not in name and name not in ('.','..')
    CAPTURES.mkdir(parents=True,exist_ok=True)
    with (CAPTURES/(name+'.lock')).open('a') as lease:
        fcntl.flock(lease,fcntl.LOCK_EX)
        yield CAPTURES/name
