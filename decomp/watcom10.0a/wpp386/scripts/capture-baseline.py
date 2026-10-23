#!/usr/bin/env python3
"""Explicit, approved original capture; never called by native fixtures."""

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[4]
BASE = Path.home() / ".ai-tmp/msvc2/watcom10.0a/wpp386/help"
PROGRAM = r"R:\toolchains\watcom10.0a\binnt\WPP386.EXE"
WATCOM = r"R:\toolchains\watcom10.0a"
IDENTITIES = {
    "binb/WPP386.EXE": "647f4e754cd27fc76b3c44a47227ae3baa4ff2e2f253a8ec27216414653b1e6f",
    "binnt/WPP386.EXE": "8fee7d6ab3f2c06a0a874135e3898386898679e7dd53993fcc508e296364767d",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(directory):
    result = []
    for path in sorted(directory.rglob("*")):
        if path.is_symlink() or not path.is_file():
            raise ValueError(f"unexpected non-regular output: {path}")
        result.append({"path": str(path.relative_to(directory)),
                       "size": path.stat().st_size, "sha256": sha(path)})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute-approved-two-baselines", action="store_true")
    args = parser.parse_args()
    if not args.execute_approved_two_baselines:
        parser.error("explicit capture flag required; this is not a fixture runner")
    for path in (PROGRAM, WATCOM):
        if "\\\\" in path or path[1:3] != ":\\":
            raise ValueError("non-canonical Windows path")
    tc = ROOT / "toolchains/watcom10.0a"
    for name, digest in IDENTITIES.items():
        if sha(tc / name) != digest:
            raise ValueError(f"original identity changed: {name}")
    environment = {
        "HOME": str(Path.home()), "USER": "root", "LOGNAME": "root",
        "PATH": "/usr/bin:/bin", "WINEPREFIX": str(Path.home() / ".ai-tmp/vc1.0-capture/wine"),
        "WINEDEBUG": "-all", "LC_ALL": "C", "LANG": "C", "TZ": "UTC",
        "WATCOM": WATCOM,
    }
    wine = shutil.which("wine", path=environment["PATH"])
    if wine is None:
        raise ValueError("Wine executable not found")
    command = [wine, PROGRAM, "?"]
    BASE.mkdir(parents=True, exist_ok=True)
    with (BASE / "capture.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        work = BASE / "work"
        work.mkdir(exist_ok=True)
        if inventory(work):
            raise ValueError("owned work directory must be empty before capture")
        evidence = Path(tempfile.mkdtemp(prefix="unmodified-", dir=BASE))
        receipt = {
            "kind": "explicit-unmodified-original-baseline",
            "accepted_functions": 0, "masked": False,
            "command_argv": command, "cwd": str(work),
            "windows_cwd_expected": r"S:\watcom10.0a\wpp386\help\work",
            "host_launch_environment": environment,
            "windows_environment_observed": False,
            "image_sha256": IDENTITIES,
            "capture_script_sha256": sha(Path(__file__)), "runs": [],
        }
        (evidence / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print(evidence, flush=True)
        for name in ("reference-a", "reference-b"):
            setup_started = time.perf_counter()
            run = evidence / name
            run.mkdir()
            (run / "stdin.bin").write_bytes(b"")
            before = inventory(work)
            setup_seconds = time.perf_counter() - setup_started
            started = time.perf_counter()
            with (run / "stdin.bin").open("rb") as source, (run / "stdout.bin").open("wb") as out, (run / "stderr.bin").open("wb") as err:
                try:
                    process = subprocess.run(command, cwd=work, env=environment,
                                             stdin=source, stdout=out, stderr=err,
                                             timeout=30)
                    status = process.returncode
                    timed_out = False
                except subprocess.TimeoutExpired:
                    status = None
                    timed_out = True
            elapsed = time.perf_counter() - started
            check_started = time.perf_counter()
            after = inventory(work)
            raw = (run / "stdout.bin").read_bytes() + (run / "stderr.bin").read_bytes()
            actual_help = b"Usage: wpp386 [options] file [options]" in raw
            observation = {
                "run": name, "status": status, "timed_out": timed_out,
                "stdin": {"size": 0, "sha256": sha(run / "stdin.bin")},
                "input_inventory": before, "output_inventory": after,
                "stdout": {"size": (run / "stdout.bin").stat().st_size, "sha256": sha(run / "stdout.bin")},
                "stderr": {"size": (run / "stderr.bin").stat().st_size, "sha256": sha(run / "stderr.bin")},
                "actual_1994_help_marker": actual_help,
                "setup_seconds": setup_seconds, "execution_seconds": elapsed,
            }
            if after:
                shutil.copytree(work, run / "artifacts")
            observation["checks_seconds"] = time.perf_counter() - check_started
            receipt["runs"].append(observation)
            (evidence / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
            print(json.dumps(observation), flush=True)
            if timed_out or not actual_help:
                raise SystemExit("HOLD: original timed out or did not display actual help; no alternate invocation attempted")
            for output in after:
                (work / output["path"]).unlink()
        a, b = receipt["runs"]
        receipt["raw_runs_equal"] = all(a[key] == b[key] for key in ("status", "stdout", "stderr", "output_inventory"))
        receipt["normalizations"] = []
        (evidence / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        if not receipt["raw_runs_equal"]:
            raise SystemExit("HOLD: raw baseline runs differ")


if __name__ == "__main__":
    main()
