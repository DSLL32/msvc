#!/usr/bin/env python3
"""Create only the reviewed, fixed-size four-site WASM compatibility copy."""

import hashlib
import json
from pathlib import Path
import time


HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    started = time.perf_counter()
    proposal_path = HERE / "cases/help/gs-patch-proposal.json"
    proposal = json.loads(proposal_path.read_text())
    original_path = ROOT / proposal["original"]["path"]
    loader_path = ROOT / proposal["unchanged_loader_copy"]["source"]
    original = original_path.read_bytes()
    loader = loader_path.read_bytes()
    if digest(original) != proposal["original"]["sha256"] or \
            digest(loader) != proposal["unchanged_loader_copy"]["sha256"]:
        raise SystemExit("original identity changed")
    patched_path = ROOT / proposal["proposed_patched_payload"]["path"]
    copied_loader_path = ROOT / proposal["unchanged_loader_copy"]["destination"]
    if patched_path.exists() or copied_loader_path.exists():
        raise SystemExit("destination exists; refusing to replace any prior evidence")
    data = bytearray(original)
    for patch in proposal["patches"]:
        offset = int(patch["file_offset"], 0)
        old, new = bytes.fromhex(patch["old"]), bytes.fromhex(patch["new"])
        if len(old) != len(new) or bytes(data[offset:offset + len(old)]) != old:
            raise SystemExit("patch bytes or fixed size changed")
        data[offset:offset + len(old)] = new
    if digest(data) != proposal["proposed_patched_payload"]["sha256"]:
        raise SystemExit("patched-copy hash differs from approved proposal")
    tail = int(proposal["resource_tail"]["file_offset"], 0)
    if data[tail:] != original[tail:]:
        raise SystemExit("resource tail changed")
    patched_path.parent.mkdir(parents=True, exist_ok=True)
    copied_loader_path.parent.mkdir(parents=True, exist_ok=True)
    with patched_path.open("xb") as output:
        output.write(data)
    with copied_loader_path.open("xb") as output:
        output.write(loader)
    if original_path.read_bytes() != original or loader_path.read_bytes() != loader:
        raise SystemExit("original files changed during copy creation")
    receipt = {
        "schema": 1,
        "kind": "approved compatibility copy creation only; no execution",
        "proposal_sha256": digest(proposal_path.read_bytes()),
        "original": proposal["original"],
        "patched_payload": proposal["proposed_patched_payload"],
        "unchanged_loader_copy": proposal["unchanged_loader_copy"],
        "patches": proposal["patches"],
        "resource_tail": proposal["resource_tail"],
        "all_original_bytes_unchanged": True,
        "elapsed_seconds": time.perf_counter() - started,
        "execution_gate": "HOLD until persistent all-invocation selector/ownership/thread/rearm guard review",
        "application_acceptance": "none",
    }
    (HERE / "cases/help/gs-copy-creation.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
