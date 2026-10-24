#!/usr/bin/env python3
"""Validate raw CRT-entry trial evidence; promotion is a separate reviewed step."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import importlib.util

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
INPUTS = ("cases/lifecycle/candidate.json", "functions.json", "runtime.json",
          "graph-metadata.json", "build/c1n32_l.dll", "build/c1xx.map",
          "build/runtime.json", "build/trial.json")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def checked_receipt(directory):
    receipt = json.loads((directory / "receipt.json").read_text())
    require(receipt["native_sha256"] == sha(PROJECT / "build/native/c1xx-host"), "stale native executable")
    require(set(receipt["snapshot_inputs"]) == set(INPUTS), "incomplete trial snapshots")
    for item in INPUTS:
        frozen = directory / ("rebuilt-runtime.json" if item == "build/runtime.json" else Path(item).name)
        require(frozen.read_bytes() == (PROJECT / item).read_bytes(), "stale/wrong trial " + item)
        require(receipt["snapshot_inputs"][item] == sha(frozen), "wrong snapshot hash " + item)
    for name, record in receipt["artifacts"].items():
        require(Path(name).name == name, "artifact escaped capture directory")
        path = directory / name
        require(path.is_file() and sha(path) == record["sha256"] and path.stat().st_size == record["size"],
                "raw artifact changed " + name)
    return receipt


def check(original, native, require_audit=True, audit_path=None):
    cfg = json.loads((PROJECT / "functions.json").read_text())
    candidate = json.loads((PROJECT / cfg.get("candidate_path", "cases/lifecycle/candidate.json")).read_text())
    if candidate["case"] == "minimal":
        require(not candidate.get("prospective_only"), "prospective raw-only observations cannot promote functions")
        require(not candidate.get('test_arena_reset_seed'),'Reset data auxiliary cannot promote a function')
        require(not candidate.get('test_action_seed'),'action data auxiliary cannot promote a function')
        require(not candidate.get('test_comment_seed'),'comment data auxiliary cannot promote a function')
        require(not candidate.get('test_group_seed'),'group data auxiliary cannot promote a function')
        require(not candidate.get('test_pch_seed'),'PCH data auxiliary cannot promote functions')
        require(not candidate.get('test_prototype_seed'),'prototype data auxiliary cannot promote a function')
        require(not candidate.get('test_pool_seed'),'pool constructor data auxiliary cannot promote a function')
        require(candidate.get('test_pdb_seed') is None,'PDB data auxiliary cannot promote a function')
        require(not candidate.get('test_seed') or (candidate['trial']=='_next_argument' and candidate.get('test_seed')=='restore-nonzero'),'auxiliary/seeded executions cannot promote a function outside explicitly exercised restore-only iterator')
        require(not candidate.get('test_token_seed'),'token data auxiliary cannot promote a function')
        require(not candidate.get('test_deferred_seed'),'deferred-work data auxiliary cannot promote a function')
        require(not candidate.get('test_constructor_seed'),'constructor data-seed auxiliary cannot promote a function')
        filename={'_InvokeCompilerPass@12':'check-minimal.py',
                  '_initialize_compiler_pass':'check-projection.py',
                  '_checked_malloc':'check-projection.py',
                  '_register_cleanup':'check-callback.py',
                  '_initialize_deferred_work':'check-callback.py',
                  '_initialize_pdb_context_01':'check-callback.py',
                  '_initialize_pdb_context_02':'check-callback.py',
                  'CompilerPool::Initialize':'check-callback.py',
                  '_initialize_type_ranks':'check-callback.py',
                  '_initialize_generated_comment':'check-callback.py',
                  '_main':'check-callback.py',
                  '_initialize_work_buffers':'check-callback.py',
                  '_reserve_work_buffer':'check-callback.py',
                  '_initialize_intermediate_files':'check-callback.py',
                  '_process_early_option':'check-callback.py',
                  '_initialize_frontend_hook':'check-callback.py',
                  '_initialize_compiler_environment':'check-callback.py',
                  '_initialize_default_arena':'check-callback.py',
                  'CompilerArena::Reserve':'check-callback.py',
                  '_calculate_arena_reservation':'check-callback.py',
                  'CompilerArena::Reset':'check-callback.py',
                  '_initialize_pool_arenas':'check-callback.py',
                  'CompilerPool::BindArena':'check-callback.py',
                  'CompilerPool::Refill':'check-callback.py',
                  'CompilerArena::AllocatePages':'check-callback.py',
                  'CompilerArena::CommitExtent':'check-callback.py',
                  '_process_compiler_options':'check-callback.py',
                  '_push_declaration_context':'check-callback.py',
                  '_initialize_include_aliases':'check-callback.py',
                  '_initialize_checksums':'check-callback.py',
                  '_initialize_multibyte_categories':'check-callback.py',
                  '_initialize_lowercase_table':'check-callback.py',
                  '_read_env_arguments':'check-callback.py',
                  '_new_option_vector':'check-callback.py',
                  '_next_argument':'check-callback.py',
                  '_match_option':'check-callback.py',
                  '_initialize_frontend_01':'check-callback.py'}.get(candidate['trial'])
        if (candidate['trial'] or '').startswith('_initialize_frontend_'):
            filename='check-callback.py'
        if (candidate['trial'] or '').startswith('_initialize_output_buffer_'):
            filename='check-callback.py'
        require(filename is not None,'no reviewed decoder for this minimal-case candidate')
        spec = importlib.util.spec_from_file_location("minimal",Path(__file__).with_name(filename))
        decoder = importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
        return decoder.check(original,native,require_audit,audit_path)
    if candidate["trial"] in ("__CRT_INIT@12", "import:_initterm", "_DllMain@12"):
        decoder_name = {"__CRT_INIT@12":"check-initializer.py", "import:_initterm":"check-import.py", "_DllMain@12":"check-completion.py"}[candidate["trial"]]
        spec = importlib.util.spec_from_file_location("initializer", Path(__file__).with_name(decoder_name))
        decoder = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(decoder)
        return decoder.check(original, native, require_audit, audit_path)
    o, n = checked_receipt(original), checked_receipt(native)
    require(o["kind"] == "explicit-original-capture" and o["mode"] == "masked" and o["exit"] == 125,
            "original has no masked frontier")
    require(n["kind"] == "native-gdb-frontier", "native has no raw frontier capture")
    trial = json.loads((original / "trial.json").read_text())
    candidate = json.loads((original / "candidate.json").read_text())
    require(trial["trial"] == candidate["trial"] == "__DllMainCRTStartup@12", "wrong first CRT trial")
    require(trial["accepted"] == candidate["accepted"] + [candidate["trial"]], "trial absent from audited vertices")
    require(trial["ranges"] == candidate["ranges"] and trial["candidate_sha256"] == sha(original / "candidate.json"),
            "mask and audited candidate differ")
    for item, digest in trial["bound_inputs"].items():
        require(sha(PROJECT / item) == digest, "trial build/input changed " + item)
    target = ROOT / "toolchains/5.10/bin/C1XX.DLL"
    provider = ROOT / "toolchains/5.10/bin/MSVCRT.DLL"
    require(sha(original / "loaded-target.bin") == o["target_sha256"] == sha(target) == trial["target"]["sha256"],
            "actual loaded target differs")
    require(sha(original / "loaded-msvcrt.bin") == o["canonical_crt_sha256"] == sha(provider) ==
            "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", "actual provider differs")
    require(sha(original / "dll-capture.exe") == o["helper_sha256"] == sha(PROJECT / "build/dll-capture.exe"),
            "original debugger helper changed")
    text = (original / "events.txt").read_text()
    frontiers = re.findall(r"^frontier rva=([0-9a-f]{8})$", text, re.M)
    traps = re.findall(r"^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$", text, re.M)
    modules = re.findall(r"^module name=c1n32_l.dll actual=([0-9a-f]{8}) preferred=([0-9a-f]{8}) entry=([0-9a-f]{8})$", text, re.M)
    require(len(frontiers) == len(modules) == 1 and len(traps) == 2, "missing/ambiguous raw trial frontier")
    require([int(row[0]) for row in traps] == [0, 1] and
            [int(row[1], 16) for row in traps] == [0x2e3c6, 0x2e46a] and
            int(frontiers[0], 16) == 0x2e46a, "raw trial did not advance to CRT_INIT")
    require("failure " not in text and "mask snapshot=relocated-text.bin baseline=0 ranges=2" in text,
            "original mask transport failed")
    require(int(modules[0][0], 16) == int(modules[0][1], 16) == 0x10400000,
            "nonpreferred-base certification is not part of this first trial")
    snapshot = (original / "relocated-text.bin").read_bytes()
    require(len(snapshot) == 0xb7670 and snapshot[0x2e3c6 - 0x1000] == 0x53, "bad loaded snapshot")
    expected_mask = bytearray(b"\xcc" * len(snapshot))
    for row in trial["ranges"]:
        first, last = row["start_rva"] - 0x1000, row["end_rva"] - 0x1000
        require(0 <= first < last <= len(snapshot), "mask range escaped code")
        expected_mask[first:last] = snapshot[first:last]
        if row["entry"]:
            expected_mask[first] = 0xcc
    require((original / "masked-text.bin").read_bytes() == expected_mask,
            "actual full mask/restored hot+cold bytes differ")
    state_bytes = (original / "state-001.bin").read_bytes()
    require(len(state_bytes) == 0xa4, "missing complete raw target state")
    typed_original = {"process_attached": struct.unpack_from("<i", state_bytes, 0xc)[0],
                      "raw_dll_entry": struct.unpack_from("<I", state_bytes, 0x24)[0]}
    state = json.loads((native / "state.json").read_text())
    require(state["symbol"] == "_CRT_INIT", "native raw frontier differs")
    transcript = (native / "gdb.txt").read_text()
    require("Program received signal SIGTRAP" in transcript and "in _CRT_INIT (" in transcript,
            "native GDB transcript has no real CRT_INIT trap")
    require(state["registration"] == {"name": "C1XX.DLL", "path": "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
                                     "sha256": sha(target), "flags": 2}, "wrong native module provenance")
    stack = [int(word, 16) for word in traps[-1][2].split(",")]
    require(stack[1:4] == [0x10400000, 1, 0] and state["args"] == {
        "module": state["registered_token"], "reason": 1, "reserved": 0} and state["registered_token"] != 0,
        "typed lifecycle arguments differ")
    require(typed_original == state["target_state"] == {"process_attached": 0, "raw_dll_entry": 0},
            "typed CRT input state differs; non-null callback needs its own identity evidence")
    binary = PROJECT / "build/native/c1xx-host"
    symbols = subprocess.check_output(["nm", "-S", "--defined-only", str(binary)], text=True)
    match = re.search(r"^([0-9a-f]+) ([0-9a-f]+) [Tt] _CRT_INIT$", symbols, re.M)
    require(match is not None, "missing actual native CRT_INIT symbol")
    start, size = (int(word, 16) for word in match.groups())
    require(start < state["pc"] <= start + size, "native frontier PC escaped CRT_INIT")
    opcode = subprocess.check_output(["objdump", "-d", "--start-address=" + str(state["pc"] - 1),
                                      "--stop-address=" + str(state["pc"]), str(binary)], text=True)
    require("int3" in opcode, "native frontier is not an actual INT3 stub")
    for stream in ("stdout.bin", "stderr.bin"):
        require((original / stream).read_bytes() == (native / stream).read_bytes(), "partial output differs " + stream)
    if require_audit:
        proof = json.loads((audit_path or PROJECT / "build/audit-proof.json").read_text())
        require(proof["trial"] == trial["trial"] and proof["trial_sha256"] == sha(original / "trial.json"),
                "audit did not check this exact immutable trial")
        auditor = ROOT / "tooling/graph-audit/target/release/graph-audit"
        require(proof["auditor_sha256"] == sha(auditor), "auditor changed after proof")
        report = proof["report"]
        chunks = [[0x1042e3c6, 0x1042e46a], [0x1046acb3, 0x1046acef]]
        require(report["checked_functions"] == 1 and trial["trial"] in report["functions"] and
                report["owned_code_chunks"]["original"]["functions"][trial["trial"]] == chunks,
                "audit did not prove one complete logical fragmented CRT function")
    return {"trial": trial["trial"], "frontier": "__CRT_INIT@12", "raw_frontier_rva": 0x2e46a,
            "typed_state": typed_original, "whole_mask_bytes_checked": len(snapshot),
            "audit_checked": require_audit, "durably_accepted": 0,
            "promotion_requires_parent_review": True}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    parser.add_argument("--frontier-only", action="store_true", help="diagnostic only; not promotion-ready")
    args = parser.parse_args()
    print(json.dumps(check(args.original, args.native, not args.frontier_only), indent=2))
