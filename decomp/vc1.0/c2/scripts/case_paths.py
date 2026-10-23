"""Fixed repository Windows drives and isolated C2 capture/native namespaces."""
from contextlib import contextmanager
import fcntl
from pathlib import Path, PureWindowsPath

ROOT = Path(__file__).resolve().parents[4]
SCRATCH = Path.home()/'.ai-tmp/msvc2'
C2_SCRATCH = SCRATCH/'vc1.0/c2'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'


def windows(path):
    path = Path(path).resolve()
    for drive, root in [('R:', ROOT), ('S:', SCRATCH)]:
        try:
            relative = path.relative_to(root)
        except ValueError:
            continue
        return drive+'\\'+str(relative).replace('/', '\\')
    raise ValueError('Original build/capture path must be under fixed R: or S: roots: '+str(path))


def verify_prefix():
    assert (PREFIX/'system.reg').is_file(), 'Shared Wine prefix must already exist'
    assert (PREFIX/'dosdevices/r:').resolve() == ROOT
    assert (PREFIX/'dosdevices/s:').resolve() == SCRATCH


def canonical_parts(canonical):
    path = PureWindowsPath(canonical)
    assert path.drive.upper() == 'S:' and path.root == '\\', canonical
    parts = path.parts[1:]
    assert len(parts) == 3 and parts[:2] == ('vc1.0', 'c2'), canonical
    assert parts[2] not in ('.', '..'), canonical
    return parts


def native_work(directory, canonical):
    work = directory.joinpath(*canonical_parts(canonical))
    work.mkdir(parents=True)
    return work, dict(RECON_DRIVE_R=str(ROOT), RECON_DRIVE_S=str(directory),
                      RECON_CANONICAL_CWD=canonical)


@contextmanager
def capture_lock():
    C2_SCRATCH.mkdir(parents=True, exist_ok=True)
    with (C2_SCRATCH/'capture.lock').open('a') as lease:
        fcntl.flock(lease, fcntl.LOCK_EX)
        verify_prefix()
        yield
