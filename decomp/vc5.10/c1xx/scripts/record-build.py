#!/usr/bin/env python3
"""Keep the source contents consumed by each compiler/link command."""
import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import subprocess
import time

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
RECON = ROOT / "lib/recon"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", type=Path, required=True)
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--link", action="store_true")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    files = set(PROJECT.glob("src/*.[ch]")) | set((PROJECT / "scripts").glob("*.h")) | set((RECON / "include").glob("*.h"))
    for item in command:
        path = Path(item)
        if path.suffix in (".c", ".h") and path.is_file():
            files.add(path.resolve())
    if args.native:
        files |= set((RECON / "native").rglob("*.h"))
        files |= set((RECON / "src").glob("*.c"))
    if args.link:
        files |= set((PROJECT / ("build/native" if args.native else "build")).glob("*.build.json"))
    before = {str(path.relative_to(ROOT)): path.read_bytes() for path in sorted(files)}
    started = time.perf_counter()
    result = subprocess.run(command)
    elapsed = time.perf_counter() - started
    if result.returncode:
        return result.returncode
    if any((ROOT / name).read_bytes() != data for name, data in before.items()):
        raise ValueError("source/header changed during compile/link; rebuild required")
    store = PROJECT / "build/source-content"
    store.mkdir(parents=True, exist_ok=True)
    # Only the short content-store write is serialized; compiler jobs stay -j4.
    with (store / ".lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        for data in before.values():
            path = store / digest(data)
            if path.exists():
                if path.read_bytes() != data:
                    raise ValueError("immutable source content changed")
            else:
                with path.open("xb") as stream:
                    stream.write(data)
    record = {"schema": 1, "command": command, "seconds": elapsed,
              "artifact": str(args.artifact), "artifact_sha256": digest(args.artifact.read_bytes()),
              "source_content": {name: digest(data) for name, data in before.items()},
              "scope": "Project source/header dependency superset and native shared implementation; content frozen at command start and unchanged at completion"}
    args.record.write_text(json.dumps(record, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
