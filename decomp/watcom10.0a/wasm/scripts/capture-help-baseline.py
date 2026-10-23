#!/usr/bin/env python3
"""Explicit, one-shot original capture; never called by native fixtures."""

import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time


HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(directory):
    result = []
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            result.append({"path": str(path.relative_to(directory)),
                           "symlink": os.readlink(path)})
        elif path.is_file():
            result.append({"path": str(path.relative_to(directory)),
                           "size": path.stat().st_size, "sha256": digest(path)})
    return result


def main():
    started = time.perf_counter()
    plan_path = HERE / "cases/help/execution-plan.json"
    plan = json.loads(plan_path.read_text())
    original = plan["original"]
    identity = json.loads((HERE / "cases/image-metadata.json").read_text())
    for field, sha_field in (("target", "target_sha256"), ("loader", "loader_sha256")):
        if digest(ROOT / identity[field]) != identity[sha_field]:
            raise SystemExit("original identity changed; refusing capture")
    directory = Path(original["cwd"])
    if directory.exists():
        raise SystemExit("capture directory already exists; refusing to overwrite evidence")
    directory.mkdir(parents=True)
    (directory / "stdin.bin").write_bytes(b"")
    before = inventory(directory)
    command = original["argv"]
    if "\\\\" in command[1]:
        raise SystemExit("noncanonical Windows path separators")
    environment = original["host_environment_allowlist"]
    setup_elapsed = time.perf_counter() - started
    launch_started = time.perf_counter()
    timed_out = False
    with (directory / "stdin.bin").open("rb") as stdin, \
            (directory / "stdout.bin").open("wb") as stdout, \
            (directory / "stderr.bin").open("wb") as stderr:
        process = subprocess.Popen(command, cwd=directory, env=environment,
                                   stdin=stdin, stdout=stdout, stderr=stderr,
                                   start_new_session=True)
        try:
            status = process.wait(timeout=original["execution_timeout_seconds"])
        except subprocess.TimeoutExpired:
            timed_out = True
            # Only this explicitly owned capture's fresh process group.
            # Do not invoke wineserver -k or touch another process group.
            os.killpg(process.pid, signal.SIGTERM)
            try:
                status = process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                status = process.wait()
    execution_elapsed = time.perf_counter() - launch_started
    check_started = time.perf_counter()
    after = inventory(directory)
    receipt = {
        "schema": 1, "case": "help", "kind": "unmasked-original-baseline",
        "plan_sha256": digest(plan_path), "argv": command, "cwd": str(directory),
        "host_environment": environment,
        "windows_environment": "not observed; no claim of complete Windows environment",
        "original_identities": {field: identity[field] for field in
            ("target", "target_sha256", "loader", "loader_sha256")},
        "timeout_seconds": original["execution_timeout_seconds"],
        "timed_out": timed_out, "returncode": status,
        "inventory_before": before, "inventory_after": after,
        "timing_seconds": {
            "setup": setup_elapsed, "execution": execution_elapsed,
            "checks": time.perf_counter() - check_started,
            "total": time.perf_counter() - started,
        },
        "acceptance": "none; no masks, ABI proof or reconstructed functions",
    }
    (directory / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))
    if timed_out:
        raise SystemExit("original capture timed out; hold for review")


if __name__ == "__main__":
    main()
