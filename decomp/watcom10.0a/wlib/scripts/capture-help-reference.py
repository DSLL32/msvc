#!/usr/bin/env python3
"""Explicit original-only help capture; never called by native fixture targets."""

import argparse
import hashlib
import json
import os
import pathlib
import signal
import subprocess
import time

from metadata import EXPECTED_SHA256, ORIGINAL


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(directory):
    result = []
    for path in sorted(directory.rglob("*")):
        item = dict(path=str(path.relative_to(directory)))
        if path.is_symlink():
            item.update(kind="symlink", target=os.readlink(path))
        elif path.is_dir():
            item.update(kind="directory")
        elif path.is_file():
            item.update(kind="file", size=path.stat().st_size, sha256=digest(path))
        else:
            item.update(kind="other")
        result.append(item)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=pathlib.Path,
                        help="new scratch receipt directory; must not already exist")
    args = parser.parse_args()
    if digest(ORIGINAL) != EXPECTED_SHA256:
        raise SystemExit("original hash mismatch")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    cwd = output / "unmasked"
    cwd.mkdir()
    payload = b"\n" * 64
    (output / "stdin.bin").write_bytes(payload)
    env = dict(os.environ)
    env.update(WINEPREFIX="/root/.ai-tmp/vc1.0-capture/wine", WINEDEBUG="-all",
               WATCOM=r"R:\toolchains\watcom10.0a", LANG="C", LC_ALL="C", TZ="UTC0")
    removed = {name: env.pop(name) for name in ("WLIB", "WLIB_OPTIONS") if name in env}
    (output / "host-environment.json").write_text(json.dumps(env, indent=2, sort_keys=True) + "\n")
    (output / "host-environment.json").chmod(0o600)
    (output / "removed-host-defaults.json").write_text(json.dumps(removed, indent=2) + "\n")
    (output / "removed-host-defaults.json").chmod(0o600)
    argv = ["/usr/bin/wine", r"R:\toolchains\watcom10.0a\binnt\WLIB.EXE", "-?"]
    before = inventory(cwd)
    start = time.monotonic()
    timed_out = False
    with (output / "stdout.bin").open("wb") as stdout, (output / "stderr.bin").open("wb") as stderr:
        process = subprocess.Popen(argv, cwd=cwd, env=env, stdin=subprocess.PIPE,
                                   stdout=stdout, stderr=stderr, start_new_session=True)
        try:
            process.communicate(payload, timeout=30)
        except subprocess.TimeoutExpired:
            timed_out = True
            if os.getpgid(process.pid) != process.pid:
                raise RuntimeError("capture no longer owns its process group")
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
    elapsed = time.monotonic() - start
    receipt = dict(status="timeout-hold" if timed_out else "captured-unmasked-baseline",
                   original=str(ORIGINAL), original_sha256_before=EXPECTED_SHA256,
                   original_sha256_after=digest(ORIGINAL), argv=argv, cwd=str(cwd),
                   host_environment_file="host-environment.json",
                   host_environment_sha256=digest(output / "host-environment.json"),
                   target_windows_environment="not observed; host launch environment only",
                   stdin=dict(kind="pipe", file="stdin.bin", size=64, sha256=digest(output / "stdin.bin")),
                   stdout=dict(kind="regular file", file="stdout.bin", size=(output / "stdout.bin").stat().st_size,
                               sha256=digest(output / "stdout.bin")),
                   stderr=dict(kind="regular file", file="stderr.bin", size=(output / "stderr.bin").stat().st_size,
                               sha256=digest(output / "stderr.bin")),
                   exit_status=process.returncode, timeout_seconds=30, elapsed_seconds=elapsed,
                   files_before=before, files_after=inventory(cwd),
                   application_functions_accepted=0, masked_replay=False)
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({key: receipt[key] for key in ("status", "exit_status", "elapsed_seconds", "stdout", "stderr", "files_after")}, indent=2))
    if timed_out or receipt["original_sha256_after"] != EXPECTED_SHA256:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
