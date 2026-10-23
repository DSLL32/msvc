"""Immutable shared C/header evidence for explicit reconstruction builds.

Only compare the live tree across the actual build window. Historical receipts
verify their own archived bytes and never require an evolving worktree match.
"""
import gzip
import hashlib
import io
import json
from pathlib import Path
import tarfile
import subprocess

BINARY = Path(__file__).resolve().parents[1]
SHARED = BINARY.parents[2] / 'lib/recon'
ARCHIVES = BINARY / 'evidence/shared-build-inputs'


def inputs(root=SHARED):
    return {str(p.relative_to(root)): p.read_bytes()
            for folder in ('include', 'native', 'src')
            for p in sorted((root / folder).rglob('*'))
            if p.is_file() and p.suffix in ('.c', '.h', '.inc')}


def signature(files):
    rows = {name: dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
            for name, data in sorted(files.items())}
    raw = (json.dumps(dict(schema=1, root='lib/recon', files=rows),
                      sort_keys=True, indent=2) + '\n').encode()
    return hashlib.sha256(raw).hexdigest(), raw


def committed_inputs():
    repo = BINARY.parents[2]
    reference = json.loads((BINARY / 'cases/shared-build-reference.json').read_text())
    actual = subprocess.check_output(['git', 'rev-parse', reference['commit'] + ':lib/recon'],
                                     cwd=repo, text=True).strip()
    assert actual == reference['tree'], 'pinned shared tree differs'
    listing = subprocess.check_output(['git', 'ls-tree', '-r', '-z', reference['commit'], '--',
        'lib/recon/include', 'lib/recon/native', 'lib/recon/src'], cwd=repo)
    files = {}
    for entry in listing.split(b'\0'):
        if not entry: continue
        identity, path = entry.split(b'\t', 1)
        mode, kind, object_id = identity.decode().split()
        name = path.decode()
        if Path(name).suffix not in ('.c', '.h', '.inc'): continue
        assert kind == 'blob' and mode in ('100644', '100755')
        files[name[len('lib/recon/'):]] = subprocess.check_output(
            ['git', 'cat-file', 'blob', object_id], cwd=repo)
    assert files and 'include/recon.h' in files and 'src/crt.c' in files
    return files, reference


def snapshot():
    files, reference = committed_inputs()
    digest, manifest = signature(files)
    ARCHIVES.mkdir(exist_ok=True)
    path = ARCHIVES / (digest + '.tar.gz')
    manifest_path = ARCHIVES / (digest + '.json')
    if not path.exists():
        output = io.BytesIO()
        with tarfile.open(fileobj=output, mode='w') as archive:
            for name, data in sorted(files.items()):
                member = tarfile.TarInfo(name)
                member.size = len(data)
                member.mode = 0o644
                archive.addfile(member, io.BytesIO(data))
        path.write_bytes(gzip.compress(output.getvalue(), mtime=0))
        manifest_path.write_bytes(manifest)
    assert manifest_path.read_bytes() == manifest, 'immutable shared manifest differs'
    with tarfile.open(path, 'r:gz') as archive:
        saved = {member.name: archive.extractfile(member).read()
                 for member in archive.getmembers()}
    assert saved == files, 'immutable shared archive differs'
    return dict(manifest=str(manifest_path.relative_to(BINARY)),
                manifest_sha256=digest,
                archive=str(path.relative_to(BINARY)),
                archive_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                files=len(files), committed_reference=reference['commit'],
                committed_tree=reference['tree'])


def current_signature():
    return signature(inputs())[0]


def materialize(record):
    manifest = BINARY / record['manifest']
    assert hashlib.sha256(manifest.read_bytes()).hexdigest() == record['manifest_sha256']
    path = BINARY / record['archive']
    assert hashlib.sha256(path.read_bytes()).hexdigest() == record['archive_sha256']
    rows = json.loads(manifest.read_text())['files']
    directory = BINARY / 'build/shared-inputs' / record['manifest_sha256']
    with tarfile.open(path, 'r:gz') as archive:
        assert set(archive.getnames()) == set(rows)
        for member in archive.getmembers():
            assert member.isfile() and '..' not in Path(member.name).parts
            data = archive.extractfile(member).read()
            assert len(data) == rows[member.name]['bytes']
            assert hashlib.sha256(data).hexdigest() == rows[member.name]['sha256']
            target = directory / member.name
            if target.exists():
                assert target.read_bytes() == data, 'immutable compiled input changed'
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
                target.chmod(0o444)
    assert signature(inputs(directory))[0] == record['manifest_sha256']
    return directory


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--makefile', type=Path, required=True)
    parser.add_argument('--pin-commit', help='explicit committed shared slice reviewed with its maintainer')
    args = parser.parse_args()
    if args.pin_commit:
        repo = BINARY.parents[2]
        commit = subprocess.check_output(['git', 'rev-parse', args.pin_commit + '^{commit}'],
                                         cwd=repo, text=True).strip()
        tree = subprocess.check_output(['git', 'rev-parse', commit + ':lib/recon'],
                                       cwd=repo, text=True).strip()
        path = BINARY / 'cases/shared-build-reference.json'
        reference = json.loads(path.read_text())
        if (commit, tree) != (reference['commit'], reference['tree']):
            reference.setdefault('previous_references', []).append(
                dict(commit=reference['commit'], tree=reference['tree']))
            reference.update(commit=commit, tree=tree,
                verification='Explicit reviewed committed slice; refresh invoked with --pin-commit')
            path.write_text(json.dumps(reference, indent=2) + '\n')
    record = snapshot()
    directory = materialize(record)
    value = 'RECON = ' + str(directory.relative_to(BINARY)) + '\n'
    args.makefile.parent.mkdir(parents=True, exist_ok=True)
    args.makefile.write_text(value)
