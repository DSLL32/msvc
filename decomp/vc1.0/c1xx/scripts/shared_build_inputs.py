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

BINARY = Path(__file__).resolve().parents[1]
SHARED = BINARY.parents[2] / 'lib/recon'
ARCHIVES = BINARY / 'evidence/shared-build-inputs'


def inputs():
    return {str(p.relative_to(SHARED)): p.read_bytes()
            for folder in ('include', 'native', 'src')
            for p in sorted((SHARED / folder).rglob('*'))
            if p.is_file() and p.suffix in ('.c', '.h', '.inc')}


def signature(files):
    rows = {name: dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
            for name, data in sorted(files.items())}
    raw = (json.dumps(dict(schema=1, root='lib/recon', files=rows),
                      sort_keys=True, indent=2) + '\n').encode()
    return hashlib.sha256(raw).hexdigest(), raw


def snapshot():
    files = inputs()
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
                files=len(files))


def current_signature():
    return signature(inputs())[0]
