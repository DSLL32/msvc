"""Fixed original Windows drives and isolated native C1XX case namespaces."""
from pathlib import Path, PureWindowsPath

BINARY = Path(__file__).resolve().parents[1]
ROOT = BINARY.parents[2]
SCRATCH = Path.home()/'.ai-tmp/msvc2'
CAPTURE_ROOT = SCRATCH/'vc1.0/c1xx'
PREFIX = Path.home()/'.ai-tmp/vc1.0-capture/wine'


def windows(path):
    path = Path(path).resolve()
    for drive, root in [('R:', ROOT), ('S:', SCRATCH)]:
        try:
            relative = path.relative_to(root)
        except ValueError:
            continue
        return drive+'\\'+str(relative).replace('/', '\\')
    raise ValueError('Original path must be under fixed R: or S: roots: '+str(path))


def canonical_parts(canonical):
    path = PureWindowsPath(canonical)
    assert path.drive.upper() == 'S:' and path.root == '\\', canonical
    parts = path.parts[1:]
    assert len(parts) == 3 and parts[:2] == ('vc1.0', 'c1xx'), canonical
    assert parts[2] not in ('.', '..'), canonical
    return parts


def native_work(directory, canonical):
    work = directory.joinpath(*canonical_parts(canonical))
    work.mkdir(parents=True, exist_ok=False)
    return work, dict(RECON_DRIVE_R=str(ROOT), RECON_DRIVE_S=str(directory),
                      RECON_CANONICAL_CWD=canonical)


def paths(config, name):
    case = config['cases'][name]
    fixture = (BINARY/case.get('fixture', '../fixtures/c1xx-cpp-basic')).resolve()
    reference = BINARY/case.get('native_reference', 'cases/native-cpp-basic')
    frontiers = BINARY/case.get('frontiers', 'cases/frontiers/'+name)
    return case, fixture, reference, frontiers
