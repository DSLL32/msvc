#!/usr/bin/env python3
"""Bind shared Rust support facts into the finite first-frontier proposal.

Run `make -j4 audit` first. This composes metadata only: it neither discovers
instructions nor launches an original or a native case.
"""
import hashlib
import json
from pathlib import Path


PROJECT = Path(__file__).resolve().parents[1]
REPO = PROJECT.parents[2]


def load(path):
    return json.loads(path.read_text())


def bind(path):
    return {"path": str(path),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    witness_path = PROJECT / "harness/startup-witness.json"
    witness = load(witness_path)
    report_path = PROJECT / "build/graph-audit.json"
    facts = load(report_path)
    support = facts["decoded_support_instructions"]
    authorization = {}
    for owner in witness["support_owners"]:
        name = owner["name"]
        fact = support["functions"][name]
        original = fact["original"]
        assert fact["classification"] == "CRT"
        assert original["checked_ret_cleanup_set"] is None
        authorization[name] = {
            "role": "proved-support", "cleanup": owner["abi"]["cleanup"],
            "record_sha256": original["record_sha256"],
            "abi": {"kind": owner["abi"]["kind"],
                    "evidence": fact["evidence"]}}
    report = {
        "kind": "original-support", "project": str(PROJECT),
        "config": bind(PROJECT / ".build/functions.json"),
        "runtime": bind(PROJECT / ".build/runtime-original.json"),
        "rebuilt_runtime": bind(PROJECT / ".build/rebuilt-runtime.json"),
        "metadata": bind(PROJECT / "build/graph-metadata.json"),
        "rebuilt": bind(PROJECT / "build/WPP386.EXE"),
        "map": bind(PROJECT / "build/wpp386.map"),
        "map_format": "watcom10", "check_ret": False,
        "expected_inputs": support["inputs"], "authorization": authorization}

    # Exact operands reached before the first App call. BSS has its separately
    # checked zeroing rule; no whole-allocation writable permission is granted.
    data = []

    def span(name, start, end, write):
        data.append(dict(name=name, start=start, end=end, write=write,
                         evidence=bind(witness_path)))

    for name, start, end in [
        ("OS identifier", 0xb8d78, 0xb8d79),
        ("stack top", 0xb8d5e, 0xb8d62),
        ("stack low", 0xb8d5a, 0xb8d5e),
        ("program name pointer", 0xb8d42, 0xb8d46),
        ("command tail pointer", 0xb8d3e, 0xb8d42),
        ("far gateway", 0xb8d38, 0xb8d3e),
        ("break flag pointer", 0xb8d4a, 0xb8d4e),
        ("copyright pointer", 0xb8d4e, 0xb8d52),
        ("environment block pointer", 0xb8d46, 0xb8d4a),
        ("environment array pointer", 0xb8d52, 0xb8d56),
        ("saved DS", 0x664f2, 0x664f4),
        ("current break", 0xb8d6a, 0xb8d6e),
        ("startup BSS zero", 0xbc438, 0xbf6a0),
        ("initializer table", 0xbc420, 0xbc432),
        ("saved SS", 0xb975c, 0xb975e),
        ("stderr flag byte", 0xb8dc1, 0xb8dc2)]:
        span(name, start, end, True)
    span("first FILE flag", 0xb8d8c, 0xb8d90, False)
    reports = [report]
    paired = facts.get("decoded_instructions", {})
    if paired.get("functions"):
        candidate = dict(report)
        candidate.update(kind="paired", check_ret=True,
                         expected_inputs=paired["inputs"], authorization={})
        for name, fact in paired["functions"].items():
            original = fact["original"]
            assert original["checked_ret_cleanup_set"] == [0]
            candidate["authorization"][name] = {
                "role": "candidate", "cleanup": 0,
                "record_sha256": original["record_sha256"]}
        reports.append(candidate)
        # This scalar is introduced by MemAlloc itself; it is App heap state,
        # rather than an unbounded writable runtime data span.
        data.append(dict(name="private heap Initialized", start=0xb9774,
                         end=0xb9778, write=False,
                         evidence=bind(PROJECT / "cases/0000-help/memalloc-evidence.json")))
    catalog = {
        "schema": 1,
        "producer": bind(REPO / "tooling/graph-audit/target/release/graph-audit"),
        "reports": reports, "data": data, "startup": True,
        "max_steps": 32768}
    profile = load(PROJECT / "harness/cf-identity.json")
    profile["mode"] = "progress"
    profile["runtime_catalog"] = catalog
    output = PROJECT / "harness/help-profile.json"
    output.write_text(json.dumps(profile, indent=2) + "\n")
    print(json.dumps({"profile": bind(output), "support_report": bind(report_path),
                      "support_functions": len(authorization),
                      "scope": "static proposal; no launch or App acceptance"}, indent=2))


if __name__ == "__main__":
    main()
