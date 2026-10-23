#!/usr/bin/env python3
"""Explicit original-only DLL capture. This script is not a fixture dependency."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time

from groundwork import inspect
from provenance import freeze
from wine_tool_support import environment, windows, ROOT, PROJECT, TC, SCRATCH

TARGET = TC / "bin/C1.DLL"
CRT = TC / "bin/MSVCRT.DLL"
CRT_HASH = "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def preflight():
    current = inspect(TARGET)
    assert current == json.loads((PROJECT / "cases/inventory.json").read_text())
    assert sha(CRT) == CRT_HASH
    data = TARGET.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    optional = pe + 24
    relocation, size = struct.unpack_from("<2I", data, optional + 96 + 5 * 8)

    def raw(rva):
        for section in current["sections"]:
            if section["rva"] <= rva < section["rva"] + section["raw_size"]:
                return section["raw_offset"] + rva - section["rva"]
        raise ValueError("unbacked relocation RVA")

    at, end = raw(relocation), raw(relocation) + size
    sites = []
    while at < end:
        page, length = struct.unpack_from("<2I", data, at)
        assert length >= 8 and at + length <= end
        for word in struct.unpack_from("<%dH" % ((length - 8) // 2), data, at + 8):
            kind, offset = word >> 12, word & 0xfff
            if not kind:
                continue
            assert kind == 3, "unsupported relocation type"
            site = page + offset
            assert not site <= current["entry_rva"] < site + 4
            sites.append(site)
        at += length
    assert data[raw(current["entry_rva"])] == 0x53
    # The target imports no DLL that directly imports C1 back. This is a narrow
    # pre-entry-cycle check, not a claim about arbitrary dynamically loaded code.
    for item in current["imports"]:
        local = current["local_import_files"][item["module"]]
        if local:
            dependency = inspect(ROOT / local)
            assert not any(entry["module"].lower() in ("c1.dll", "c1n32_l.dll")
                           for entry in dependency["imports"])
    return {"entry_byte": 0x53, "entry_relocation_overlap": False,
            "tls_directory": current["tls_directory"], "relocation_sites": len(sites),
            "local_import_cycle_into_target": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("baseline", "masked"))
    parser.add_argument("--name", required=True, help="new scratch case directory name")
    parser.add_argument("--case", choices=("lifecycle", "minimal"), default="lifecycle")
    args = parser.parse_args()
    assert args.name.replace("-", "").replace("_", "").isalnum()
    checks = preflight()
    output = SCRATCH / "vc5.10/c1" / args.name
    output.mkdir(parents=True, exist_ok=False)
    candidate = json.loads((PROJECT / ("cases/" + args.case + "/candidate.json")).read_text())
    ranges = []
    for entry in candidate["ranges"]:
        ranges.append("%x %x %d\n" % (entry["start_rva"], entry["end_rva"], entry["entry"]))
    if args.mode == "baseline":
        for rva in (0x1202a, 0x120ce, 0x1218e, 0x12223, 0x121a9, 0x121af):
            ranges.append("%x %x 1\n" % (rva, rva + 1))
    (output / "ranges.txt").write_text("".join(ranges))
    freeze([PROJECT / "scripts/dll-capture.c", PROJECT / "scripts/capture-original.py",
            PROJECT / "scripts/wine_tool_support.py"], output / "source-snapshot.json")
    shutil.copy2(PROJECT / "build/dll-capture.exe", output / "dll-capture.exe")
    shutil.copy2(CRT, output / "MSVCRT.DLL")
    shutil.copy2(PROJECT / ("cases/" + args.case + "/candidate.json"), output / "candidate.json")
    audit_config = PROJECT / candidate.get("audit_config", "functions.json")
    shutil.copy2(audit_config, output / "functions.json")
    if candidate.get("functions"):
        binding = json.loads((PROJECT / "build/trial-audit-binding.json").read_text())
        assert all(sha(ROOT / path) == value for path, value in binding["inputs"].items())
        shutil.copy2(PROJECT / "build/trial-audit.json", output / "audit.json")
        shutil.copy2(PROJECT / "build/trial-audit-binding.json", output / "audit-binding.json")
    source_hashes = {str(path.relative_to(PROJECT)): sha(path)
                     for path in sorted((PROJECT / "src").glob("*")) if path.is_file()}
    command = ["wine", windows(output / "dll-capture.exe"), "--debug",
               windows(TARGET), windows(CRT), args.mode, "ranges.txt"]
    if args.case == "minimal":
        shutil.copy2(PROJECT / "cases/minimal/inputs/test.c", output / "test.c")
        command.append("minimal")
    started = time.perf_counter()
    with (output / "stdout.bin").open("wb") as stdout, (output / "stderr.bin").open("wb") as stderr:
        result = subprocess.run(command, cwd=output, env=environment(), stdout=stdout,
                                stderr=stderr, timeout=90)
    elapsed = time.perf_counter() - started
    loaded = output / "loaded-target.bin"
    loaded_crt = output / "loaded-msvcrt.bin"
    if loaded.exists():
        assert sha(loaded) == sha(TARGET)
    if loaded_crt.exists():
        assert sha(loaded_crt) == CRT_HASH
    receipt = {"schema": 1, "kind": "explicit-original-capture",
               "candidate_path": "cases/" + args.case + "/candidate.json", "mode": args.mode,
               "command": command, "cwd": windows(output), "exit": result.returncode,
               "seconds": elapsed, "preflight": checks,
               "target_sha256": sha(TARGET), "canonical_crt_sha256": CRT_HASH,
               "helper_sha256": sha(output / "dll-capture.exe"),
               "candidate_sha256": sha(output / "candidate.json"),
               "functions_sha256": sha(output / "functions.json"),
               "native_sha256": sha(PROJECT / "build/native/c1-host"),
               "rebuilt_sha256": sha(PROJECT / "build/c1.dll"),
               "source_hashes": source_hashes,
               "audit_binding_sha256": sha(output / "audit-binding.json") if (output / "audit-binding.json").exists() else None,
               "artifacts": {p.name: {"sha256": sha(p), "size": p.stat().st_size}
                             for p in sorted(output.iterdir()) if p.is_file()}}
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"output": str(output), "exit": result.returncode, "seconds": elapsed}))
    return 0 if result.returncode in (0, 125) else 1


if __name__ == "__main__":
    raise SystemExit(main())
