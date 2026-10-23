#!/usr/bin/env python3
"""Check first CRT-entry trial's raw next frontier and typed state; no promotion."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess

spec = importlib.util.spec_from_file_location("pair", Path(__file__).with_name("check-pair.py"))
pair = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pair)
PROJECT = pair.PROJECT


def check(original, native, audit_directory=None):
    o, n = pair.checked_receipt(original), pair.checked_receipt(native)
    assert o["kind"] == "explicit-original-capture" and o["mode"] == "masked" and o["exit"] == 125
    assert n["kind"] == "native-gdb-frontier"
    candidate = json.loads((original / "candidate.json").read_text())
    config = json.loads((original / "functions.json").read_text())
    assert candidate["functions"] and config["driver"], "trial itself is absent"
    tool_path = "tooling/graph-audit/target/release/graph-audit"
    captured_inputs = []
    for directory, receipt in ((original, o), (native, n)):
        assert receipt["audit_binding_sha256"] == pair.sha(directory / "audit-binding.json")
        captured_binding = json.loads((directory / "audit-binding.json").read_text())
        execution_inputs = {path: value for path, value in captured_binding["inputs"].items() if path != tool_path}
        assert all(pair.sha(pair.ROOT / path) == value for path, value in execution_inputs.items())
        assert pair.sha(directory / "audit.json") == captured_binding["report_sha256"]
        captured_inputs.append(execution_inputs)
    assert captured_inputs[0] == captured_inputs[1]
    execution_inputs = captured_inputs[0]
    assert o["helper_sha256"] == pair.sha(PROJECT / "build/dll-capture.exe")
    audit_directory = audit_directory or PROJECT / "build"
    binding = json.loads((audit_directory / "trial-audit-binding.json").read_text())
    assert all(pair.sha(pair.ROOT / path) == value for path, value in binding["inputs"].items())
    assert {path: value for path, value in binding["inputs"].items() if path != tool_path} == execution_inputs
    assert pair.sha(audit_directory / "trial-audit.json") == binding["report_sha256"]
    report = json.loads((audit_directory / "trial-audit.json").read_text())
    assert {f["name"] for f in candidate["functions"]} == {f["name"] for f in config["driver"]}
    chunks = sorted(tuple(chunk) for f in candidate["functions"] for chunk in f["chunks"])
    assert chunks == sorted((r["start_rva"], r["end_rva"]) for r in candidate["ranges"])
    assert all(start < end for start, end in chunks)
    assert all(left[1] <= right[0] for left, right in zip(chunks, chunks[1:]))
    expected_owned = {f["name"]: [[0x10600000 + a, 0x10600000 + b] for a, b in f["chunks"]]
                      for f in candidate["functions"]}
    owned = report["owned_code_chunks"]["original"]
    assert owned["functions"] == expected_owned
    assert owned["logical_function_count"] == report["checked_functions"] == len(candidate["functions"])
    assert owned["interval_count"] == len(chunks)
    assert owned["declared_code_bytes"] == sum(b - a for a, b in chunks)
    assert {"call_graph", "ret_cleanup", "extent_ownership", "whole_extent_boundaries"} <= set(report["checks"])
    assert pair.sha(original / "loaded-target.bin") == o["target_sha256"] == config["target"]["sha256"] == pair.sha(pair.ROOT / config["target"]["path"])
    assert pair.sha(original / "loaded-msvcrt.bin") == o["canonical_crt_sha256"] == pair.sha(pair.ROOT / "toolchains/5.10/bin/MSVCRT.DLL")
    expected = candidate["expected_frontier"]
    raw = (original / "events.txt").read_text()
    frontiers = re.findall(r"^frontier rva=([0-9a-f]{8})$", raw, re.M)
    traps = re.findall(r"^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)\nlast-error=(\d+)$", raw, re.M)
    assert len(frontiers) == 1 and traps, "missing raw next frontier"
    sequence, rva, words, error = traps[-1]
    assert int(frontiers[0], 16) == int(rva, 16) == expected["rva"]
    assert "failure " not in raw
    assert int(traps[0][1], 16) == candidate["functions"][0]["chunks"][0][0]
    state = json.loads((native / "state.json").read_text())
    assert state["symbol"] == expected["native_symbol"]
    transcript = (native / "gdb.txt").read_text()
    assert "Program received signal SIGTRAP" in transcript and "in " + state["symbol"] + " (" in transcript
    symbols = subprocess.check_output(["nm", "-S", "--defined-only", str(PROJECT / "build/native/c1-host")], text=True)
    match = re.search(r"^([0-9a-f]+) ([0-9a-f]+) [Tt] " + re.escape(state["symbol"]) + "$", symbols, re.M)
    assert match
    begin, size = (int(value, 16) for value in match.groups())
    assert begin < state["pc"] <= begin + size
    opcode = subprocess.check_output(["objdump", "-d", "--start-address=" + str(state["pc"] - 1),
                                      "--stop-address=" + str(state["pc"]), str(PROJECT / "build/native/c1-host")], text=True)
    assert "int3" in opcode
    args = [int(value, 16) for value in words.split(",")][1:4]
    module = re.search(r"^module name=c1n32_l.dll actual=([0-9a-f]+) preferred=([0-9a-f]+)", raw, re.M)
    assert module and args[0] == int(module[1], 16)
    identity = state["module_identity"]
    assert identity == {"name": "C1.DLL", "path": "R:\\toolchains\\5.10\\bin\\C1.DLL",
                         "sha256": o["target_sha256"], "flags": 2}
    assert state["args"]["module"] == state["module_token"] != 0
    assert [state["args"][name] for name in ("reason", "reserved")] == args[1:3]
    globals_data = (original / ("state-%03d.bin" % int(sequence))).read_bytes()
    attached = struct.unpack_from("<i", globals_data, 0x90)[0]
    hook = struct.unpack_from("<I", globals_data, 0x9c)[0]
    assert attached == state["crt_attached"] == 0
    assert hook == state["raw_dllmain"] == 0, "nonnull callbacks need an explicit identity projection"
    assert int(error) == state["win32_last_error"] == 14007
    return {"status": "fresh matching next frontier and typed CRT state",
            "trial": [f["name"] for f in candidate["functions"]],
            "frontier_rva": expected["rva"], "native_symbol": state["symbol"],
            "state": ["module identity/token", "reason/reserved", "CRT attach count",
                      "null raw callback", "Win32 last error"],
            "rust_checked_functions": report["checked_functions"],
            "rust_owned_original_bytes": owned["declared_code_bytes"],
            "current_auditor_sha256": binding["inputs"][tool_path],
            "current_audit_report_sha256": binding["report_sha256"],
            "execution_receipts": {"original": pair.sha(original / "receipt.json"),
                                   "native": pair.sha(native / "receipt.json")},
            "promotion": "held pending orchestrator first-candidate review"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    result = check(args.original, args.native)
    (PROJECT / "build/trial-validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
