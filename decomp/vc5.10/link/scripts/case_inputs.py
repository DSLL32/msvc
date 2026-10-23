"""Explicit cached case inputs and fixed R:/S: transport; never runs originals."""
from contextlib import contextmanager
import fcntl
import hashlib
import json
from pathlib import Path
import shutil

PROJECT=Path(__file__).resolve().parents[1]
REPO=PROJECT.parents[2]
SCRATCH=Path.home()/'.ai-tmp/msvc2'
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()


def load(case,configuration):
    assert case in configuration['cases']
    if case=='help':return dict(case=case,arguments=['/?'],inputs=[])
    path=PROJECT/'cases'/case/'case.json';manifest=json.loads(path.read_text())
    assert manifest['case']==case and manifest['arguments']==configuration['cases'][case]['arguments']
    assert manifest['schema']==1
    for item in manifest['inputs']:
        source=PROJECT/item['path']
        assert source.resolve().is_relative_to((PROJECT/'cases'/case).resolve())
        assert Path(item['name']).name==item['name'] and item['name'] not in ('.','..')
        assert source.stat().st_size==item['bytes'] and sha(source)==item['sha256']
    return manifest


def windows_work(case):
    assert case in ('hello-win32','hello-dll')
    return 'S:\\vc5.10\\link\\case-work\\'+case


@contextmanager
def workspace(case,manifest):
    """Keep relative file lookups and any returned absolute paths on identical CWD inputs.

    The case lock protects this one owned directory, without changing any drive.
    Raw reference artifacts are copied out before the next preparation.
    """
    assert case in ('hello-win32','hello-dll')
    parent=SCRATCH/'vc5.10/link/case-work';parent.mkdir(parents=True,exist_ok=True)
    with (parent/(case+'.lock')).open('a')as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        work=parent/case
        assert not work.is_symlink()
        if work.exists():shutil.rmtree(work)
        work.mkdir()
        for item in manifest['inputs']:shutil.copyfile(PROJECT/item['path'],work/item['name'])
        yield work


def native_environment(case):
    if case=='help':return {}
    return dict(RECON_DRIVE_R=str(REPO),RECON_DRIVE_S=str(SCRATCH),
        RECON_CANONICAL_CWD=windows_work(case),LIB=r'R:\toolchains\5.10\lib')


def execution(manifest):
    """Execution-bearing fields; prose/provenance changes do not alter argv/input."""
    return {key:manifest[key]for key in ('case','arguments','inputs','outputs')}


def save_artifacts(work,destination,manifest):
    output=destination/'artifacts';output.mkdir()
    inputs={item['name']:item for item in manifest['inputs']}
    for name,item in inputs.items():
        source=work/name
        assert source.stat().st_size==item['bytes']and sha(source)==item['sha256'],'input was modified: '+name
    for path in work.iterdir():
        if path.name not in inputs:
            assert path.is_file(),'unexpected output directory'
            shutil.copyfile(path,output/path.name)
