#!/usr/bin/env python3
"""Check the retained first guarded reference without launching any process."""

import gzip
import hashlib
import json
from pathlib import Path
import re


HERE = Path(__file__).resolve().parents[1]
CASE = HERE / "cases/help-patched-flat-resource"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    inventory = json.loads((CASE / "first-capture-inventory.json").read_text())
    receipt_path = CASE / inventory["raw_receipt"]["path"]
    if sha(receipt_path.read_bytes()) != inventory["raw_receipt"]["sha256"]:
        raise SystemExit("retained raw receipt changed")
    receipt = json.loads(receipt_path.read_text())
    events = gzip.decompress((CASE / inventory["raw_events"]["stored_path"]).read_bytes())
    if sha(events) != inventory["raw_events"]["raw_sha256"]:
        raise SystemExit("decompressed raw events changed")
    expected_files = {row["path"]: row for row in receipt["files"]}
    for name in ("stdout.bin", "stderr.bin", "launcher.stdout.bin", "launcher.stderr.bin"):
        data = (CASE / "reference" / name).read_bytes()
        if sha(data) != expected_files[name]["sha256"] or len(data) != expected_files[name]["bytes"]:
            raise SystemExit("retained raw stream changed: " + name)
    if sha(events) != expected_files["events.txt"]["sha256"]:
        raise SystemExit("events do not match original receipt")
    if receipt["launcher_returncode"] or receipt["outer_timeout"] or receipt["bound_inputs_changed"]:
        raise SystemExit("capture did not complete cleanly")
    lines = events.decode().splitlines()
    for prefix, count in inventory["guard_counts"].items():
        selected = [line for line in lines if line.startswith(prefix)]
        if len(selected) != count:
            raise SystemExit("guard coverage count changed: " + prefix)
        for line in selected:
            result = re.search(r"result=(\d+)", line)
            if result and int(result[1]):
                raise SystemExit("guard predicate failed: " + line)
    required = ["cf-all-relocations-verified count=2915",
                "actual-CreateFileA-observer-rearmed-after-validated-CF",
                "actual-CreateFileA-phase=loader-open",
                "actual-CreateFileA-phase=first-postlaunch-open",
                "guard-final-segments ds=002b gs=0063 calls=43",
                "normal-exit-continue result=1",
                "normal-owned-exit signaled=1 wait-result=00000000 actual-status=1 event-status=1",
                "diagnostic-end timeout=0 child-status=1"]
    for text in required:
        if not any(line.startswith(text) for line in lines):
            raise SystemExit("missing original observation: " + text)
    print("guarded WASM help reference: raw hashes, 43 complete guard lifetimes, 86 rearm checks and owned exit pass")


if __name__ == "__main__":
    main()
