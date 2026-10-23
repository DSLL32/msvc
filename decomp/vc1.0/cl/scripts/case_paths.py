"""Fixed repository/scratch drives and isolated native CL case namespaces."""
from pathlib import Path,PureWindowsPath

ROOT=Path(__file__).resolve().parents[4]
SCRATCH=Path.home()/'.ai-tmp/msvc2'
PREFIX=Path.home()/'.ai-tmp/vc1.0-capture/wine'


def windows(path):
    path=Path(path).resolve()
    for drive,root in [('R:',ROOT),('S:',SCRATCH)]:
        try:relative=path.relative_to(root)
        except ValueError:continue
        return drive+'\\'+str(relative).replace('/','\\')
    raise ValueError('Original build/capture path must be inside fixed R: or S: roots: '+str(path))


def canonical_cwd(case_id):
    assert case_id and case_id not in ('.','..') and '/' not in case_id and '\\' not in case_id
    return 'S:\\vc1.0\\cl\\'+case_id


def host_path(canonical,scratch=SCRATCH):
    path=PureWindowsPath(canonical)
    assert path.root=='\\' and path.drive.upper() in ('R:','S:'),canonical
    assert not any(p in ('.','..') for p in path.parts[1:]),canonical
    return (ROOT if path.drive.upper()=='R:' else scratch).joinpath(*path.parts[1:])


def native_work(directory,case_id):
    namespace=directory/'namespace'
    work=host_path(canonical_cwd(case_id),namespace)
    work.mkdir(parents=True)
    return work,dict(RECON_DRIVE_R=str(ROOT),RECON_DRIVE_S=str(namespace),RECON_CANONICAL_CWD=canonical_cwd(case_id))
