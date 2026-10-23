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
    parser.add_argument("--restore-probe", action="store_true")
    parser.add_argument("--prospective-tail-chain", action="store_true", help="bounded raw-only input observation, never acceptance")
    parser.add_argument("--negative-value", action="store_true", help="labeled negative signed-value auxiliary input")
    parser.add_argument("--pdb-reuse", type=int, choices=(0,1), default=0, help="labeled persistent-context helper reuse branch")
    args = parser.parse_args()
    assert not args.restore_probe or args.case == "minimal"
    assert not args.negative_value or args.restore_probe
    assert not args.pdb_reuse or args.restore_probe
    assert args.name.replace("-", "").replace("_", "").isalnum()
    assert not args.prospective_tail_chain or args.mode == "masked"
    checks = preflight()
    output = SCRATCH / "vc5.10/c1" / args.name
    output.mkdir(parents=True, exist_ok=False)
    candidate = json.loads((PROJECT / ("cases/" + args.case + "/candidate.json")).read_text())
    if candidate.get("field_map"):
        field_map = json.loads((PROJECT / candidate["field_map"]).read_text())
        probes = field_map.get("output_buffers", [])
        (output / "output-buffers.txt").write_text("".join("%x %x %x\n" %
            (row["original_object_rva"], row["constructor_rva"], row["destructor_rva"]) for row in probes))
        if args.restore_probe and field_map.get("value_copy"):
            plan = candidate["restore_probe"]["value_copy"]
            values = [plan["before_clear_source"], plan["before_clear_cached"],
                      plan["negative_source" if args.negative_value else "positive_source"], plan["cached_before_copy"]]
            words = [word for value in values for word in (value & 0xffffffff, (value >> 32) & 0xffffffff)]
            (output / "value-probe.txt").write_text(" ".join("%x" % word for word in words + [plan["flag_before"]]) + "\n")
        if args.restore_probe and field_map.get("pdb_signature"):
            plan = candidate["restore_probe"]["pdb_signature"]
            (output / "signature-probe.txt").write_text("%x %x\n" % (plan["before_clear"], plan["before_callback"]))
        if args.restore_probe and field_map.get("pdb_context"):
            (output / "pdb-context-probe.txt").write_text("%x 2\n" % args.pdb_reuse)
        if args.restore_probe and field_map.get("pools"):
            (output / "pools-probe.txt").write_text("".join("%x\n" % row["original_object_rva"] for row in field_map["pools"]))
    if args.restore_probe and candidate.get("restore_probe", {}).get("pool_member"):
        (output / "pool-member-probe.txt").write_text("126fc %x\n" % len(field_map["pools"]))
    if args.restore_probe and field_map.get("empty_record_prototype"):
        (output / "prototype-probe.txt").write_text("58f50 14 01 a0\n")
    if args.restore_probe and field_map.get("pch_state"):
        (output / "pch-state-probe.txt").write_text("58f68 44\n")
    if args.restore_probe and candidate.get("restore_probe", {}).get("pch_member"):
        (output / "pch-member-probe.txt").write_text("11e2a 58f68 44\n")
    if args.restore_probe and field_map.get("main_fields"):
        (output / "main-fields-probe.txt").write_text("5f3d8 5f2e0 5ede4 5f508 40 55c34\n")
    if candidate.get('allocation_observers'):
        assert candidate['allocation_observers']['rvas']==[0x12889,0x1288b,0x128a2,0x128a4,0x1283f,0x12860]
        (output/'allocation-observers.txt').write_text(''.join('%x\n'%rva for rva in candidate['allocation_observers']['rvas']))
    if candidate.get('parser_observers'):
        assert candidate['parser_observers']==dict(getenv_return_rva=0x128ec,parser_entry_rva=0x12900,parser_return_rva=0x128d2)
        (output/'internal-switch-observers.txt').write_text('128ec\n')
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
    if args.prospective_tail_chain:
        assert args.case == "minimal"
        from prospective import freeze as freeze_prospective
        freeze_prospective(output, candidate)
    elif candidate.get("functions"):
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
        command.append("minimal-restore" if args.restore_probe else "minimal")
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
    receipt = {"schema": 1, "kind": "original-prospective-tail-chain" if args.prospective_tail_chain else "explicit-original-capture",
               "candidate_path": "cases/" + args.case + "/candidate.json",
               "probe": "restore" if args.restore_probe else "normal",
               "value_copy_sign": ("negative" if args.negative_value else "positive") if args.restore_probe else None,
               "pdb_reuse": args.pdb_reuse if args.restore_probe else None, "mode": args.mode,
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
