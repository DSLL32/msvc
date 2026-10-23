#!/usr/bin/env python3
"""Run the common Rust audit on the actual immutable trial and bind its inputs."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
TOOL = ROOT / "tooling/graph-audit/target/release/graph-audit"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    candidate_path = PROJECT / "cases/lifecycle/candidate.json"
    candidate = json.loads(candidate_path.read_text())
    paths = [candidate_path, PROJECT / candidate["audit_config"], PROJECT / "runtime.json",
             PROJECT / "runtime-rebuilt.json", PROJECT / "build/c1.map",
             PROJECT / "build/c1.dll", PROJECT / "build/native/c1-host",
             ROOT / "toolchains/5.10/bin/C1.DLL", TOOL]
    return {str(path.relative_to(ROOT)): sha(path) for path in paths}


def main():
    subprocess.run(["make", "graph-audit-tool"], cwd=PROJECT, check=True,
                   stdout=subprocess.DEVNULL)
    before = inputs()
    candidate = json.loads((PROJECT / "cases/lifecycle/candidate.json").read_text())
    command = [str(TOOL), "--profile", "cl", "--discovery", "cfg", "--repo", str(ROOT),
               "--project", str(PROJECT), "--original", str(ROOT / "toolchains/5.10/bin/C1.DLL"),
               "--rebuilt", str(PROJECT / "build/c1.dll"), "--map", str(PROJECT / "build/c1.map"),
               "--config", str(PROJECT / candidate["audit_config"]),
               "--runtime", str(PROJECT / "runtime.json"),
               "--rebuilt-runtime", str(PROJECT / "runtime-rebuilt.json"),
               "--check-ret", "--check-ownership", "--check-extents"]
    start = time.perf_counter()
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE, text=True)
    seconds = time.perf_counter() - start
    assert inputs() == before, "audit inputs changed during execution"
    report = json.loads(result.stdout)
    expected = {function["name"]: [[0x10600000 + a, 0x10600000 + b]
                                   for a, b in function["chunks"]]
                for function in candidate["functions"]}
    owned = report["owned_code_chunks"]["original"]
    assert owned["functions"] == expected
    assert owned["logical_function_count"] == report["checked_functions"] == len(expected)
    assert owned["interval_count"] == sum(len(chunks) for chunks in expected.values())
    assert owned["declared_code_bytes"] == sum(b - a for chunks in expected.values() for a, b in chunks)
    assert set(report["functions"]) == set(expected)
    assert set(report["checks"]) >= {"call_graph", "ret_cleanup", "extent_ownership", "whole_extent_boundaries"}
    output = PROJECT / "build/trial-audit.json"
    output.write_text(result.stdout)
    binding = {"schema": 1, "inputs": before, "report_sha256": sha(output),
               "command": command, "seconds": seconds}
    (PROJECT / "build/trial-audit-binding.json").write_text(json.dumps(binding, indent=2) + "\n")
    print(json.dumps({"checked_functions": report["checked_functions"],
                      "owned_original_bytes": owned["declared_code_bytes"], "seconds": seconds}))


if __name__ == "__main__":
    main()
