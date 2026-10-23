#!/usr/bin/env python3
"""Explicit first approved patched-path capture; never called by fixtures."""

import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time


HERE = Path(__file__).resolve().parents[1]
PLAN = HERE / "cases/help-patched-flat-resource/guarded-capture-plan.json"
APPROVED_PLAN = "8022bc9f67f2e43ae524dc117aa675e9c74dead1911b1c001749311b68d542e2"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    started = time.perf_counter()
    if digest(PLAN) != APPROVED_PLAN:
        raise SystemExit("capture plan differs from reviewed first-run plan")
    plan = json.loads(PLAN.read_text())
    helper = plan["helper"]
    bindings = [(helper["path"], helper["sha256"]),
                (helper["frozen_packet"], helper["frozen_packet_sha256"]),
                (helper["source"], helper["source_sha256"]),
                (helper["header"], helper["header_sha256"]),
                (helper["build_receipt"], helper["build_receipt_sha256"])]
    targets = plan["targets"]
    for name in ("loader", "payload", "raw_payload"):
        bindings.append((targets[name], targets[name + "_sha256"]))
    bindings.extend((row["path"], row["sha256"]) for row in plan["inputs"])
    for path, expected in bindings:
        if digest(Path(path)) != expected:
            raise SystemExit("bound capture input changed: " + path)
    directory = Path(plan["host_cwd"])
    if directory.exists():
        raise SystemExit("capture folder exists; refusing to overwrite or retry")
    directory.mkdir(parents=True)
    setup_elapsed = time.perf_counter() - started
    launch_started = time.perf_counter()
    timed_out = False
    with (directory / "launcher.stdout.bin").open("wb") as stdout, \
            (directory / "launcher.stderr.bin").open("wb") as stderr:
        process = subprocess.Popen(plan["argv"], cwd=directory,
                                   env=plan["host_environment"], stdin=subprocess.DEVNULL,
                                   stdout=stdout, stderr=stderr, start_new_session=True)
        try:
            status = process.wait(timeout=plan["deadline"]["host_outer_seconds"])
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(process.pid, signal.SIGTERM)
            try:
                status = process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                status = process.wait()
    execution_elapsed = time.perf_counter() - launch_started
    check_started = time.perf_counter()
    changes = [path for path, expected in bindings if digest(Path(path)) != expected]
    events_path = directory / "events.txt"
    events = events_path.read_text(errors="replace") if events_path.exists() else ""
    receipt = {
        "schema": 1, "case": plan["case"],
        "kind": "first explicitly approved guarded patched-path original capture",
        "plan_path": str(PLAN), "plan_sha256": APPROVED_PLAN,
        "argv": plan["argv"], "cwd": str(directory),
        "host_environment": plan["host_environment"],
        "Windows_environment": "not observed; no full environment equivalence claim",
        "helper": helper,
        "targets": targets,
        "inputs": plan["inputs"],
        "launcher_unix_pid": process.pid, "launcher_returncode": status,
        "outer_timeout": timed_out,
        "bound_inputs_changed": changes,
        "normal_owned_exit_observed": "normal-owned-exit signaled=1" in events,
        "files": [{"path": path.name, "bytes": path.stat().st_size, "sha256": digest(path)}
                  for path in sorted(directory.iterdir()) if path.is_file()],
        "timing_seconds": {"setup": setup_elapsed, "execution": execution_elapsed,
                           "checks": time.perf_counter() - check_started,
                           "total": time.perf_counter() - started},
        "review_status": "raw outcome preserved; first result needs review before any repeat",
        "application_acceptance": "zero; no application masking or accepted functions",
    }
    (directory / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({key: receipt[key] for key in
          ("launcher_returncode", "outer_timeout", "bound_inputs_changed", "normal_owned_exit_observed",
           "timing_seconds", "files")}, indent=2))
    if timed_out or status or changes or not receipt["normal_owned_exit_observed"]:
        raise SystemExit("capture held after first outcome; no retry or fallback")


if __name__ == "__main__":
    main()
