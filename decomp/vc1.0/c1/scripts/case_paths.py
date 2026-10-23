"""Stable Windows names and isolated native case namespaces."""
from pathlib import Path, PureWindowsPath

ROOT = Path(__file__).resolve().parents[4]
SCRATCH = Path.home()/'.ai-tmp/msvc2'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'


def windows(path):
    path = Path(path).resolve()
    for drive, root in [('R:',ROOT),('S:',SCRATCH)]:
        try:
            relative = path.relative_to(root)
        except ValueError:
            continue
        return drive+'\\'+str(relative).replace('/','\\')
    raise ValueError('Original capture/build path must be under fixed R: or S: roots: '+str(path))


def canonical_parts(canonical):
    path = PureWindowsPath(canonical)
    assert path.drive.upper() == 'S:' and path.root == '\\', canonical
    parts = path.parts[1:]
    assert len(parts) == 3 and parts[:2] == ('vc1.0','c1'), canonical
    assert parts[2] not in ('.','..'), canonical
    return parts


def native_work(directory, canonical):
    environment = {'RECON_DRIVE_R':str(ROOT)}
    if canonical:
        work = directory.joinpath(*canonical_parts(canonical))
        environment.update(RECON_DRIVE_S=str(directory),RECON_CANONICAL_CWD=canonical)
    else:
        work = directory/'work'
    work.mkdir(parents=True)
    return work, environment
