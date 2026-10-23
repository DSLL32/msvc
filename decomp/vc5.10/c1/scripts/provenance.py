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


def reuse_native_sources(directory, destination, binary, candidate_path, rebuilt, config):
    """Reuse a verified ELF's real inputs, never current shared worktree bytes."""
    receipt_path = directory / "receipt.json"
    receipt = json.loads(receipt_path.read_text())
    assert receipt["kind"] == "native-gdb-frontier" and not receipt.get("build_source_reuse")
    for name, item in receipt["artifacts"].items():
        path = directory / name
        assert path.stat().st_size == item["size"] and sha(path) == item["sha256"]
    check(directory)
    assert receipt["native_sha256"] == sha(binary), "different ELF"
    assert receipt["candidate_sha256"] == sha(candidate_path), "different candidate"
    assert receipt["rebuilt_sha256"] == sha(rebuilt), "different Win32 DLL"
    assert receipt["functions_sha256"] == sha(config), "different function trial"
    manifest = directory / "source-snapshot.json"
    records = json.loads(manifest.read_text())
    prefix = str(PROJECT.relative_to(ROOT)) + "/"
    owned = {str(path.relative_to(ROOT)) for path in native_sources() if path.is_relative_to(PROJECT)}
    assert owned == {name for name in records if name.startswith(prefix)}, "owned source set changed"
    assert all(sha(ROOT / name) == records[name]["sha256"] for name in owned), "owned source content changed"
    destination.write_bytes(manifest.read_bytes())
    return {"directory": str(directory), "receipt_sha256": sha(receipt_path),
            "source_manifest_sha256": sha(manifest), "native_sha256": sha(binary),
            "policy": "same verified ELF and owned sources; original immutable shared inputs retained"}
