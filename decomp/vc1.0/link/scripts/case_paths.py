"""Fixed repository/shared-scratch names for explicit captures and native fixtures."""
from pathlib import Path, PureWindowsPath

ROOT = Path(__file__).resolve().parents[4]
SCRATCH = Path.home()/'.ai-tmp/msvc2'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'
PROGRAM = r'R:\toolchains\1.0-nt\bin\LINK.EXE'
LIBRARY = r'R:\toolchains\1.0-nt\lib'


def windows(path):
    path = Path(path).resolve()
    for drive, root in [('R:',ROOT),('S:',SCRATCH)]:
        try:
            relative = path.relative_to(root)
        except ValueError:
            continue
        return drive+'\\'+str(relative).replace('/','\\')
    raise ValueError('Capture/build path must be below fixed R: or S: roots: '+str(path))


def canonical_parts(canonical):
    path = PureWindowsPath(canonical)
    assert path.drive.upper()=='S:' and path.root=='\\', canonical
    parts = path.parts[1:]
    assert len(parts)==3 and parts[:2]==('vc1.0','link'), canonical
    assert parts[2] not in ('.','..'), canonical
    return parts


def native_work(directory,canonical):
    work=directory.joinpath(*canonical_parts(canonical))
    work.mkdir(parents=True)
    return work,dict(RECON_DRIVE_R=str(ROOT),RECON_DRIVE_S=str(directory),
                     RECON_CANONICAL_CWD=canonical)
