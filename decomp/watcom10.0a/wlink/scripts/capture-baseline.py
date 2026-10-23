#!/usr/bin/env python3
"""Explicit unmasked '?' capture. Never called by fixtures or build targets."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

from inventory import ROOT, TOOLCHAIN, inventory


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def files(directory):
    result = []
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            result.append({"path": str(path.relative_to(directory)), "symlink": os.readlink(path)})
        elif path.is_file():
            result.append({"path": str(path.relative_to(directory)), "bytes": path.stat().st_size, "sha256": sha(path)})
    return result


def main():
    started = time.monotonic()
    identities = inventory()
    binary = Path(__file__).resolve().parents[1]
    plan_path = binary / "cases/help/architecture.json"
    plan = json.loads(plan_path.read_text())["reference_capture"]
    command = plan["command"]
    if command != ["wine", "R:\\toolchains\\watcom10.0a\\.patched\\WLINK.EXE", "?"]:
        raise SystemExit("Capture command differs from the reviewed baseline")
    scratch = Path(plan["cwd"])
    scratch.mkdir(parents=True, exist_ok=True)
    capture = Path(tempfile.mkdtemp(prefix="baseline-", dir=scratch))
    work = capture / "work"
    work.mkdir()
    environment = os.environ.copy()
    for name in plan["environment_absent"]:
        environment.pop(name, None)
    environment.update(plan["environment_overrides"])
    environment_path = capture / "host-environment.json"
    descriptor = os.open(environment_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(descriptor, "w") as output:
        json.dump(environment, output, indent=2, sort_keys=True)
        output.write("\n")
    known = []
    for relative in ("binnt/WLINK.LNK", "binb/WLSYSTEM.LNK", "binnt/WLINK.HLP"):
        path = TOOLCHAIN / relative
        known.append({"path": str(path.relative_to(ROOT)), "exists": path.exists(), "sha256": sha(path) if path.is_file() else None, "actual_read_observed": False})
    before = files(work)
    setup_elapsed = time.monotonic() - started
    timed_out = False
    run_started = time.monotonic()
    with (capture / "stdout.bin").open("wb") as stdout, (capture / "stderr.bin").open("wb") as stderr:
        process = subprocess.Popen(command, cwd=work, env=environment, stdin=subprocess.DEVNULL,
                                   stdout=stdout, stderr=stderr, start_new_session=True)
        try:
            returncode = process.wait(timeout=30)
        except subprocess.TimeoutExpired:
            timed_out = True
            # Only this child-created process group, never a shared wineserver.
            os.killpg(process.pid, signal.SIGKILL)
            returncode = process.wait(timeout=5)
    run_elapsed = time.monotonic() - run_started
    check_started = time.monotonic()
    after = files(work)
    inventory()  # Fail closed if either original identity changed during execution.
    streams = {}
    for name in ("stdout", "stderr"):
        path = capture / (name + ".bin")
        streams[name] = {"path": path.name, "bytes": path.stat().st_size, "sha256": sha(path)}
    receipt = {
        "kind": "unmasked patched original baseline only; no reconstruction acceptance",
        "command": command, "cwd": str(work), "pid": process.pid, "timeout_seconds": 30,
        "returncode": returncode, "timed_out": timed_out, "stdin": "DEVNULL",
        "environment_overrides": plan["environment_overrides"],
        "environment_absent": plan["environment_absent"],
        "exact_host_environment": {"path": environment_path.name, "sha256": sha(environment_path), "mode": "0600"},
        "target_windows_environment": "not observed; host environment is not proof of target Windows environment",
        "original_sha256": identities["original_sha256"], "capture_sha256": identities["capture_sha256"],
        "patch_manifest_sha256": sha(TOOLCHAIN / ".patched/WLINK-GS-patch.json"),
        "plan_sha256": sha(plan_path), "capture_script_sha256": sha(Path(__file__)),
        "before_files": before, "after_files": after, "streams": streams,
        "known_potential_external_inputs": known,
        "external_dependencies": "actual file reads not observed; own-directory inventory is not a complete dependency trace",
        "timing_seconds": {"setup": setup_elapsed, "execution": run_elapsed, "checks": time.monotonic() - check_started}
    }
    (capture / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"capture": str(capture), "returncode": returncode, "timed_out": timed_out,
                      "streams": streams, "after_files": after, "timing_seconds": receipt["timing_seconds"]}, indent=2))
    if timed_out:
        raise SystemExit("HOLD: baseline exceeded its owned-process deadline")


if __name__ == "__main__":
    main()
