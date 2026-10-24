"""Content-addressed source input snapshots; no compiler or process execution."""
import hashlib
import json
from pathlib import Path
from state import BINARY, ROOT, sha


def inputs():
    files=list((BINARY/'src').glob('*.[ch]'))+list((BINARY/'src').glob('*.rc'))
    files.extend([BINARY/'Makefile',BINARY/'scripts/debug-run.c',BINARY/'scripts/sort-observer.inc',BINARY/'scripts/output-buffer-observer.inc',BINARY/'scripts/subsection-observer.inc',BINARY/'scripts/native-driver.c',BINARY/'scripts/native-resources.py',ROOT/'lib/recon/resources.py'])
    files.extend(BINARY/name for name in ('scripts/native-widen.c','scripts/native-widen-abi.h','scripts/native-widen.py','scripts/native-pdb.c'))
    files.extend((BINARY/'src').glob('*.cpp'))
    files.extend((BINARY/'src').glob('*.def'))
    files.append(BINARY/'scripts/native-widen-factory.cpp')
    for folder in ('src','include','native'):
        files.extend(path for path in (ROOT/'lib/recon'/folder).rglob('*') if path.suffix in ('.c','.h'))
    return sorted(set(path.resolve() for path in files))


def fingerprints():
    return {str(path.relative_to(ROOT)):sha(path) for path in inputs()}


def snapshot(output,store=None):
    blobs=(store or BINARY/'cases/source-content').resolve()
    blobs.mkdir(parents=True,exist_ok=True)
    records=[]
    for path in inputs():
        raw=path.read_bytes();digest=hashlib.sha256(raw).hexdigest();blob=blobs/digest
        if not blob.exists():blob.write_bytes(raw)
        assert sha(blob)==digest
        records.append(dict(path=str(path.relative_to(ROOT)),sha256=digest,blob=str(blob.relative_to(BINARY))))
    result=dict(schema=1,kind='compiled target/native build inputs; content retained independently of worktree',files=records)
    output.write_text(json.dumps(result,indent=2)+'\n')


def verify(path):
    result=json.loads(path.read_text())
    assert result['files']
    for item in result['files']:
        assert sha(BINARY/item['blob'])==item['sha256'], 'changed compiled-source content snapshot'
