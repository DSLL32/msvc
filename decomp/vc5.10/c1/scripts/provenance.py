"""Small immutable content cache for sources actually used by a capture."""
import hashlib
import json
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
STORE = Path.home() / ".ai-tmp/msvc2/vc5.10/c1/source-store"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def freeze(paths, destination):
    STORE.mkdir(parents=True, exist_ok=True)
    records = {}
    for path in sorted(set(paths)):
        content = path.read_bytes()
        digest = hashlib.sha256(content).hexdigest()
        cached = STORE / digest
        if cached.exists():
            assert sha(cached) == digest
        else:
            cached.write_bytes(content)
        records[str(path.relative_to(ROOT))] = {"sha256": digest, "content": str(cached)}
    destination.write_text(json.dumps(records, indent=2) + "\n")
    return records


def check(directory):
    path = directory / "source-snapshot.json"
    if not path.exists():
        return  # Historical pre-snapshot capture; its binary identities remain.
    records = json.loads(path.read_text())
    assert records
    for item in records.values():
        assert sha(Path(item["content"])) == item["sha256"], "frozen source content changed"


def native_sources():
    paths = [PROJECT / "Makefile", PROJECT / "scripts/native-host.c"]
    for directory in (PROJECT / "src", ROOT / "lib/recon/src", ROOT / "lib/recon/include", ROOT / "lib/recon/native"):
        paths.extend(path for path in directory.rglob("*") if path.is_file() and path.suffix in (".c", ".h"))
    return paths
