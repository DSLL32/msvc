#!/usr/bin/env python3
"""Explicit authored Win32 debugger probes. Not a native application fixture.

Runs only a frozen authored child/helper build after separate review. Never
launches an original Watcom binary or a GS-patched reconstruction target.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import struct
import subprocess
import time


ROOT = Path(__file__).resolve().parents[4]
SCRATCH = Path.home() / ".ai-tmp/msvc2"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    path = path.resolve()
    for base, drive in ((ROOT, "R:"), (SCRATCH, "S:")):
        if path.is_relative_to(base):
            return drive + "\\" + str(path.relative_to(base)).replace("/", "\\")
    raise ValueError("outside shared drives")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    plan = json.loads((build / "probe-plan.json").read_text())
    if sha(build / "diagnose-startup.exe") != plan["helper_sha256"] or sha(build / "guard-fixture.exe") != plan["fixture_sha256"]:
        raise ValueError("authored helper/child identity changed")
    for name, expected in plan["configuration_files"].items():
        if sha(build / name) != expected:
            raise ValueError("reviewed probe configuration changed")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env.update(WINEPREFIX=str(Path.home() / ".ai-tmp/vc1.0-capture/wine"), WINEDEBUG="-all")
    environment = output / "host-environment.json"
    environment.write_text(json.dumps(env, indent=2, sort_keys=True) + "\n")
    environment.chmod(0o600)
    results = []
    cases = [(True, name) for name in ("positive", "selector", "pointer", "capacity", "return", "thread", "unexpected")]
    cases += [(False, name) for name in ("selector", "pointer", "capacity", "return", "thread", "unexpected")]
    for guarded, name in cases:
        case = output / (("guarded-" if guarded else "unguarded-") + name)
        case.mkdir()
        cwd = case / "child"
        cwd.mkdir()
        child = case / "authored-child.exe"
        shutil.copyfile(build / "guard-fixture.exe", child)
        marker = case / "marker.bin"
        input_file = case / "input.bin"
        input_file.write_bytes(b"")
        child_mode = "positive" if name == "return" else name
        if guarded:
            config = build / ("probe-guard-wrong-return.txt" if name == "return" else "probe-guard.txt")
            raw_arguments = child_mode + ' "' + windows(marker) + '"'
            command = ["/usr/bin/wine", windows(build / "diagnose-startup.exe"), windows(child), raw_arguments,
                       windows(cwd), windows(case / "stdout.bin"), windows(case / "stderr.bin"),
                       windows(case / "events.txt"), windows(build / "probe-sites.txt"), "10000",
                       "64" if name == "positive" else "@" + windows(input_file),
                       windows(build / "probe-reads.txt"), "-", windows(config)]
        else:
            command = ["/usr/bin/wine", windows(child), child_mode, windows(marker)]
        receipt = dict(name=name, guarded=guarded, argv=command, cwd=str(cwd),
                       authored_child_sha256=sha(child), helper_sha256=plan["helper_sha256"] if guarded else None,
                       target_Windows_environment="not observed", host_environment_sha256=sha(environment))
        start = time.monotonic()
        with (case / "launcher.stdout.bin").open("wb") as out, (case / "launcher.stderr.bin").open("wb") as err:
            process = subprocess.Popen(command, cwd=cwd, env=env, stdin=subprocess.DEVNULL,
                                       stdout=out, stderr=err, start_new_session=True)
            receipt["launcher_unix_pid"] = process.pid
            try:
                process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                receipt["outer_timeout"] = True
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        receipt.update(elapsed_seconds=time.monotonic() - start, launcher_status=process.returncode)
        events = (case / "events.txt").read_text(errors="replace") if (case / "events.txt").exists() else ""
        receipt["events_sha256"] = sha(case / "events.txt") if events else None
        if marker.exists() and marker.stat().st_size == 4:
            receipt["marker_file"] = struct.unpack("<I", marker.read_bytes())[0]
        receipt["guard_reader_entries"] = len(re.findall(r"^guard-reader result=0 ", events, re.M))
        receipt["guard_owner_entries"] = len(re.findall(r"^guard-owner result=0 ", events, re.M))
        receipt["rearms"] = len(re.findall(r"^guard-step-rearmed ", events, re.M))
        receipt["reader_returns"] = len(re.findall(r"^guard-reader-return result=0 ", events, re.M))
        receipt["owner_returns"] = len(re.findall(r"^guard-owner-return result=0 ", events, re.M))
        receipt["failure_marker_reads"] = re.findall(r"^failure-read-only-dword rva=[0-9a-f]+ value=([0-9a-f]+)$", events, re.M)
        if not guarded:
            passed = process.returncode == 0 and receipt.get("marker_file") == 2
        elif name == "positive":
            passed = process.returncode == 0 and receipt.get("marker_file") == 2 and \
                     receipt["guard_reader_entries"] == 2 and receipt["guard_owner_entries"] == 2 and \
                     receipt["rearms"] == 4 and receipt["reader_returns"] == 2 and receipt["owner_returns"] == 2 and \
                     "guard-final-segments " in events and "exit status=0" in events
        else:
            passed = process.returncode != 0 and not marker.exists() and \
                     receipt["failure_marker_reads"] == ["00000000"] and \
                     "owned-exit-confirmed status=" in events and "exited-event=1 signaled=1" in events
        receipt["passed"] = passed
        (case / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        results.append(receipt)
        (output / "receipt.json").write_text(json.dumps(dict(status="running", results=results), indent=2) + "\n")
        print(json.dumps({key: receipt[key] for key in ("name", "guarded", "launcher_status", "elapsed_seconds", "passed")}))
        if not passed:
            print(events)
            raise SystemExit("authored probe failed; stopped before remaining cases")
    (output / "receipt.json").write_text(json.dumps(dict(status="all authored probes passed", results=results,
                                                         original_Watcom_or_patched_target_executed=False), indent=2) + "\n")


if __name__ == "__main__":
    main()
