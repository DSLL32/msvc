#!/usr/bin/env python3
"""Validate immutable initial-frontier evidence; does not accept functions."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from provenance import check as check_source_snapshot

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def checked_receipt(directory):
    receipt = json.loads((directory / "receipt.json").read_text())
    candidate_path = receipt.get("candidate_path", "cases/lifecycle/candidate.json")
    candidate = json.loads((PROJECT / candidate_path).read_text())
    for name, item in receipt["artifacts"].items():
        path = directory / name
        assert path.is_file() and sha(path) == item["sha256"], "raw artifact changed: " + name
        assert path.stat().st_size == item["size"]
    bindings = [("rebuilt_sha256", "build/c1.dll"),
                      ("candidate_sha256", candidate_path),
                      ("functions_sha256", candidate.get("audit_config", "functions.json"))]
    if receipt["kind"] != "explicit-original-capture":
        bindings.append(("native_sha256", "build/native/c1-host"))
    correction=None
    if receipt['kind']=='explicit-original-capture':
        from counterpart_correction import validate as validate_correction
        correction=validate_correction(directory)
    for key, file in bindings:
        if key=='rebuilt_sha256'and correction is not None:
            assert receipt[key]==correction['row']['old_rebuilt_sha256']
            assert sha(PROJECT/file)==correction['packet']['allowed_audit_deltas']['decomp/vc5.10/c1/build/c1.dll']['new']
        elif key == 'functions_sha256':
            from audit_roles import current_config
            assert receipt[key] == sha(directory/'functions.json'), 'captured config identity changed'
            assert current_config(json.loads((directory/'functions.json').read_text())) == json.loads((PROJECT/file).read_text()), 'stale or wrong trial: accepted data roles'
        else:
            assert receipt[key] == sha(PROJECT / file), "stale or wrong trial: " + key
    assert (directory / "candidate.json").read_bytes() == (PROJECT / candidate_path).read_bytes()

    for item in candidate.get("input_files", []):
        assert sha(directory / item["name"]) == item["sha256"] == sha(PROJECT / item["source"])
    check_source_snapshot(directory)
    compiled=receipt.get('compiled_build_sha256')
    if compiled:
        assert compiled==sha(directory/'compiled-build.json')
        record=json.loads((directory/'compiled-build.json').read_text())
        assert record['kind']=='native-build-source-content' and record['stable_before_after']
        assert record['native_sha256']==receipt['native_sha256']
        assert record['sources']==json.loads((directory/'source-snapshot.json').read_text())
        prefix=str(PROJECT.relative_to(ROOT))+'/'
        from provenance import native_sources
        owned={str(p.relative_to(ROOT)) for p in native_sources() if p.is_relative_to(PROJECT)}
        assert owned=={name for name in record['sources'] if name.startswith(prefix)}
        assert all(sha(ROOT/name)==record['sources'][name]['sha256'] for name in owned)
    reuse = receipt.get("build_source_reuse")
    if reuse:
        origin = Path(reuse["directory"])
        assert origin.resolve() != directory.resolve()
        assert sha(origin / "receipt.json") == reuse["receipt_sha256"]
        assert sha(origin / "source-snapshot.json") == reuse["source_manifest_sha256"]
        origin_record = json.loads((origin / "receipt.json").read_text())
        assert not origin_record.get("build_source_reuse"), "reuse chains are not accepted"
        prior = checked_receipt(origin)
        assert prior["native_sha256"] == receipt["native_sha256"] == reuse["native_sha256"]
        assert (directory / "source-snapshot.json").read_bytes() == (origin / "source-snapshot.json").read_bytes()
        records = json.loads((directory / "source-snapshot.json").read_text())
        prefix = str(PROJECT.relative_to(ROOT)) + "/"
        assert all(sha(ROOT / name) == item["sha256"] for name, item in records.items() if name.startswith(prefix))
    return receipt


def check(original, native):
    o, n = checked_receipt(original), checked_receipt(native)
    assert o["kind"] == "explicit-original-capture" and o["mode"] == "masked"
    assert n["kind"] == "native-gdb-frontier"
    candidate = json.loads((original / "candidate.json").read_text())
    expected = candidate["expected_initial_frontier"]
    assert not candidate["ranges"] and not candidate["functions"], "initial-pair checker only"
    assert o["target_sha256"] == sha(ROOT / "toolchains/5.10/bin/C1.DLL") == expected["module_sha256"]
    assert sha(original / "loaded-target.bin") == o["target_sha256"]
    assert sha(original / "loaded-msvcrt.bin") == o["canonical_crt_sha256"] == sha(ROOT / "toolchains/5.10/bin/MSVCRT.DLL")
    text = (original / "events.txt").read_text()
    frontiers = re.findall(r"^frontier rva=([0-9a-f]{8})$", text, re.M)
    traps = re.findall(r"^trap seq=0 rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$", text, re.M)
    assert len(frontiers) == len(traps) == 1, "missing or ambiguous raw frontier"
    assert int(frontiers[0], 16) == int(traps[0][0], 16) == expected["rva"]
    assert o["exit"] == 125 and "failure " not in text
    assert "mask snapshot=relocated-text.bin baseline=0 ranges=0" in text
    state = json.loads((native / "state.json").read_text())
    assert state["symbol"] == expected["native_symbol"]
    transcript = (native / "gdb.txt").read_text()
    assert "Program received signal SIGTRAP" in transcript
    assert "in " + state["symbol"] + " (" in transcript
    symbols = subprocess.check_output(["nm", "-S", "--defined-only", str(PROJECT / "build/native/c1-host")], text=True)
    match = re.search(r"^([0-9a-f]+) ([0-9a-f]+) [Tt] " + re.escape(state["symbol"]) + "$", symbols, re.M)
    assert match, "GDB frontier is not a real native symbol"
    start, size = (int(x, 16) for x in match.groups())
    assert start < state["pc"] <= start + size
    opcode = subprocess.check_output(["objdump", "-d", "--start-address=" + str(state["pc"] - 1),
                                      "--stop-address=" + str(state["pc"]), str(PROJECT / "build/native/c1-host")], text=True)
    assert "int3" in opcode, "GDB did not stop immediately after the stub INT3"
    stack = [int(x, 16) for x in traps[0][1].split(",")]
    assert [state["args"][x] for x in ("module", "reason", "reserved")] == stack[1:4]
    return {"status": "fresh initial matching frontier", "rva": expected["rva"],
            "native_symbol": state["symbol"], "accepted_functions": 0,
            "scope": "initial DLL entry and three arguments; not function or lifecycle acceptance"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    print(json.dumps(check(args.original, args.native), indent=2))
