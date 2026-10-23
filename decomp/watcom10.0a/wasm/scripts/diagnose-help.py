#!/usr/bin/env python3
"""Explicit owned-child Win32 debugger capture of the failed WASM help case."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time


HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]
SCRATCH = Path("/root/.ai-tmp/msvc2")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows(path):
    path = path.resolve()
    for base, drive in ((ROOT, "R:"), (SCRATCH, "S:")):
        try:
            return drive + "\\" + str(path.relative_to(base)).replace("/", "\\")
        except ValueError:
            pass
    raise ValueError("path is outside fixed repo/shared-scratch drives")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("helper_build", type=Path)
    args = parser.parse_args()
    started = time.perf_counter()
    build = args.helper_build.resolve()
    build_receipt = json.loads((build / "receipt.json").read_text())
    source = Path(build_receipt["source"])
    if sha(source) != build_receipt["source_sha256"] or \
            sha(build / "diagnose-startup.exe") != build_receipt["helper_sha256"]:
        raise SystemExit("helper source or executable does not match its build receipt")
    # The regular-file mode must exist in the reviewed helper source. The
    # build receipt and copied binary are retained separately below.
    if b"argv[9][0] == '@'" not in source.read_bytes():
        raise SystemExit("helper lacks the required existing regular-file stdin mode")
    plan = json.loads((HERE / "cases/help/execution-plan.json").read_text())
    original = plan["original"]
    baseline = Path(original["cwd"])
    if (baseline / "stdin.bin").read_bytes() != b"":
        raise SystemExit("baseline input changed")
    identities = json.loads((HERE / "cases/image-metadata.json").read_text())
    for field in ("target", "loader"):
        if sha(ROOT / identities[field]) != identities[field + "_sha256"]:
            raise SystemExit("original identity changed")
    out = baseline.parent / "diagnostic"
    if out.exists():
        raise SystemExit("diagnostic output exists; refusing to overwrite evidence")
    out.mkdir()
    helper = out / "diagnose-startup.exe"
    shutil.copyfile(build / helper.name, helper)
    shutil.copyfile(build / "receipt.json", out / "helper-build-receipt.json")
    shutil.copyfile(source, out / "diagnose-startup.c")
    original_image = (ROOT / identities["target"]).read_bytes()[0x2200:0x2a482]
    pe_image = (ROOT / identities["loader"]).read_bytes()
    pe_sites = [(0x10010, "pe_entry"), (0x101cc, "cf_load"),
                (0x10763, "cf_launch"), (0x110c2, "WriteFile"), (0x11146, "ExitProcess")]
    cf_sites = [
        (0x1681a, "owner_usage_call"), (0x1681f, "owner_stack_usage_return"),
        (0x1e520, "reader_entry"), (0x1e529, "resource_reader_frame"),
        (0x1e611, "resource_load_before"), (0x1e614, "resource_load_after"),
        (0x1e634, "resource_store_before"), (0x1e637, "resource_store_after"),
        (0x1e653, "resource_lgs_before"), (0x1e657, "resource_lgs_after"),
        (0x1e65b, "resource_nul_before"), (0x1e65f, "resource_nul_after"),
        (0x1e665, "reader_ret"), (0x1e6fe, "reader_return"),
    ]
    lines = ["%x %s %02x\n" % (rva, name, pe_image[0x400 + rva - 0x10000])
             for rva, name in pe_sites]
    lines.extend("cf:%x %s %02x\n" % (rva, name, original_image[rva]) for rva, name in cf_sites)
    (out / "sites.txt").write_text("".join(lines))
    (out / "reads.txt").write_text("30004\n30000\n")
    proof_input = baseline.parent / "cf-diagnostic-inputs/relocations.txt"
    if sha(proof_input) != "3bc0ac19f86f9c7dcc4926262cb26758a359adeb78b135839a15044d99b57ed3":
        raise SystemExit("CF relocation proof input changed")
    (out / "cf-proof.txt").write_text("30004 30000 1da16 28282 2915\n" + proof_input.read_text())
    command = ["/usr/bin/wine", windows(helper), original["argv"][1], "/?",
               windows(baseline), windows(out / "stdout.bin"),
               windows(out / "stderr.bin"), windows(out / "events.txt"),
               windows(out / "sites.txt"), "30000", "@" + windows(baseline / "stdin.bin"),
               windows(out / "reads.txt"), windows(out / "cf-proof.txt")]
    setup = time.perf_counter() - started
    launch = time.perf_counter()
    outer_timeout = False
    with (out / "host.stdout").open("wb") as stdout, (out / "host.stderr").open("wb") as stderr:
        process = subprocess.Popen(command, cwd=baseline,
                                   env=original["host_environment_allowlist"],
                                   stdin=subprocess.DEVNULL, stdout=stdout, stderr=stderr,
                                   start_new_session=True)
        try:
            status = process.wait(timeout=45)
        except subprocess.TimeoutExpired:
            outer_timeout = True
            os.killpg(process.pid, signal.SIGTERM)
            try:
                status = process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                status = process.wait()
    execution = time.perf_counter() - launch
    events = (out / "events.txt").read_text(errors="replace") if (out / "events.txt").exists() else ""
    receipt = {
        "schema": 1, "kind": "original-debugger-only-diagnostic", "argv": command,
        "cwd": str(baseline), "host_environment": original["host_environment_allowlist"],
        "helper_sha256": sha(helper), "helper_source_sha256": sha(out / "diagnose-startup.c"),
        "helper_build_receipt_sha256": sha(out / "helper-build-receipt.json"),
        "helper_build_receipt": build_receipt,
        "original_identities": {key: identities[key] for key in
            ("target", "target_sha256", "loader", "loader_sha256")},
        "host_returncode": status, "outer_timeout": outer_timeout,
        "owned_child_exit_event_recorded": "exit status=" in events or "owned-exit-confirmed status=" in events,
        "timing_seconds": {"setup": setup, "execution": execution,
                           "total": time.perf_counter() - started},
        "files": [{"path": p.name, "bytes": p.stat().st_size, "sha256": sha(p)}
                  for p in sorted(out.iterdir()) if p.is_file()],
        "acceptance": "none; diagnostic breakpoints only, no reconstructed functions",
    }
    (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))
    print(events)
    if outer_timeout or not receipt["owned_child_exit_event_recorded"]:
        raise SystemExit("diagnostic transport failed; actual child exit not demonstrated")


if __name__ == "__main__":
    main()
