#!/usr/bin/env python3
"""Close C2's one absolute-slot completion tail; Rust retains every CFG check.

This finite residual proves cleanup of valid normal returns only. NULL remains
non-callable. It does not discover/decode functions, accept callback bodies or
modify shared graph facts. The pending bodies are pinned reviewed originals.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

OWNER = "_c2_compile_unit"
WRITER = "_initialize_pass_functions"
ORIGINAL_SHA = "e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae"
WRITER_SOURCE_SHA = "8a613cb1d17e89494fe7f9e0310024f1ddcf195c2c727a1c9375498a6099d814"
# Exact complete hot/cold bodies independently reviewed for no entry formals,
# balanced normal-return paths and RET0. No recursive callee/body credit.
BODIES = {
    "_c2_set_argv": [(0x41eb89, 0x41eb8a, "ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e")],
    "_output_callback_4541f3": [
        (0x4541f3, 0x45427d, "1d29df2858d92e3c30781ef67725b1a52d21787697818e6e38d478f3cc5b0d2c"),
        (0x474f3b, 0x475042, "1d78b6873bb973c2f11c7b126e348f6a36ac3a94a354f608aa9160a90b388aec")],
    "_output_callback_454ba8": [
        (0x454ba8, 0x454c2f, "86733236ea02830c1f58dfb1c0c8e5f73bf562d8e1d64b386c6e4734fc9690a7"),
        (0x475637, 0x475669, "61f09dffe2dd4ce8732ed2043aaca9d40b47a1d6a21a7a9764f884b536c75ce8")],
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def semantic_sha(path):
    value = json.loads(Path(path).read_text())
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":"),
                                     ensure_ascii=False).encode()).hexdigest()


def contextual_bytes(image, address, count):
    # objdump owns PE translation. Read only the reviewed contextual intervals.
    output = subprocess.check_output(["objdump", "-s", "--start-address=" + hex(address),
                                      "--stop-address=" + hex(address + count), image], text=True)
    data = bytearray()
    for line in output.splitlines():
        # An unaligned start can produce five hex groups on a sixteen-byte row.
        match = re.match(r"^\s*[0-9a-f]+\s+((?:[0-9a-f]{2,8}\s+){1,5})", line)
        if match:
            data.extend(bytes.fromhex(match[1]))
    assert len(data) == count, "contextual byte count"
    return bytes(data)


def check(args):
    report = json.loads(Path(args.report).read_text())
    export = report["decoded_instructions"]
    config = json.loads(Path(args.config).read_text())
    contracts = json.loads(Path(args.ret_checks).read_text())
    assert export["schema"] == contracts["schema"] == 1
    for key, path in (("config_sha256", args.config), ("metadata_sha256", args.metadata)):
        assert export["inputs"][key] == semantic_sha(path), key
    for side, path in (("original", args.original), ("rebuilt", args.rebuilt)):
        image = export["inputs"]["images"][side]
        assert image["format"] == "pe32" and image["original_file_sha256"] == sha(path)
    assert sha(args.original) == ORIGINAL_SHA
    selected = set(config["accepted"])
    assert {OWNER, WRITER} <= selected
    assert set(export["functions"]) == set(report["functions"]) == selected
    assert report["checked_functions"] == len(selected)
    assert set(contracts["functions"]) == selected - {OWNER}
    ret = report["ret_cleanup_checks"]
    assert ret["input_sha256"] == sha(args.ret_checks)
    assert set(ret["functions"]) == selected - {OWNER}
    assert {"call_graph", "selected_ret_cleanup", "known_imports", "function_references",
            "callback_tables", "extent_ownership", "whole_extent_boundaries",
            "accepted_index_extents"} <= set(report["checks"])
    for name, contract in contracts["functions"].items():
        expected = contract["expected_cleanup"]
        assert expected
        for side in ("original", "rebuilt"):
            assert ret["functions"][name][side] == expected
            assert export["functions"][name][side]["checked_ret_cleanup_set"] == expected

    maps = Path(args.map).read_text()

    def address(name):
        rows = re.findall(r"^\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+" + re.escape(name) +
                          r"\s+([0-9a-fA-F]{8})\b", maps, re.M)
        assert len(rows) == 1, name
        return int(rows[0], 16)

    slots = [address("_output_slot_497e10"), address("_output_slot_497dfc")]
    assert slots[0] != slots[1]
    function = export["functions"][OWNER]
    for side in ("original", "rebuilt"):
        body = function[side]
        assert not body["declared_roots"] and not body["declared_text_data"]
        assert not body["inferred_tables"] and not body["record"]["code_chunks"]
        assert body["effective_roots"] == [body["record"]["start"]]
        assert body["checked_ret_cleanup_set"] is None
    original = function["original"]
    assert (original["record"]["start"], original["record"]["end"]) == (0x4541e7, 0x4541f3)
    assert original["local_ret_cleanup_set"] == []
    assert original["instructions"] == [
        {"address": 0x4541e7, "bytes_hex": "ff15107e4900"},
        {"address": 0x4541ed, "bytes_hex": "ff25fc7d4900"}]
    rebuilt = function["rebuilt"]
    assert rebuilt["local_ret_cleanup_set"] == [0]
    expected = [b"\x55", b"\x8b\xec", b"\xff\x15" + struct.pack("<I", slots[0]),
                b"\xff\x15" + struct.pack("<I", slots[1]), b"\x5d", b"\xc3"]
    pc = rebuilt["record"]["start"]
    for row, code in zip(rebuilt["instructions"], expected, strict=True):
        assert row == {"address": pc, "bytes_hex": code.hex()}
        pc += len(code)
    assert pc == rebuilt["record"]["end"]
    # Shared graph comparison must still retain the unresolved indirect edge.
    assert report["functions"][OWNER]["imports"] == []
    assert report["functions"][OWNER]["edges"] == ["indirect"]

    registry = {row["name"]: row for row in config["function_registry"]}
    for name, chunks in BODIES.items():
        row = registry[name]
        assert row["rva"] == chunks[0][0] - 0x400000
        assert row["code"] == [[a - 0x400000, b - 0x400000] for a, b, _ in chunks]
        assert row["definition"] == (3 if name == "_c2_set_argv" else 4)
        if name == "_output_callback_4541f3":
            # Independently qualified after the dispatcher's original review.
            # Its acceptance is never inferred from this residual.
            assert row["implementation"] == 1 and name in selected
            assert contracts["functions"][name]["expected_cleanup"] == [0]
            for side in ("original", "rebuilt"):
                assert export["functions"][name][side]["checked_ret_cleanup_set"] == [0]
        else:
            assert row["implementation"] == 0
            if name != "_c2_set_argv":
                assert name not in selected
        for first, end, digest in chunks:
            data = contextual_bytes(args.original, first, end - first)
            assert hashlib.sha256(data).hexdigest() == digest, name
        hot = contextual_bytes(args.original, chunks[0][0], chunks[0][1] - chunks[0][0])
        assert hot.endswith(b"\xc3")
    assert contextual_bytes(args.original, 0x454275, 8) == bytes.fromhex("5f5e5d5b83c410c3")
    for slot in (0x497e10, 0x497dfc):
        assert contextual_bytes(args.original, slot, 4) == bytes(4)
    # Both emitted globals are four-byte uninitialized COMMON definitions.
    # Their PE virtual zero tail is established by the linker/loader contract;
    # no bytes from a file's absent BSS tail are invented by this reader.
    symbols = subprocess.check_output(["llvm-readobj", "--symbols", args.object], text=True)
    for name in ("_output_slot_497e10", "_output_slot_497dfc"):
        rows = [b for b in re.findall(r"  Symbol \{(.*?)\n  \}", symbols, re.S)
                if re.search(r"\n    Name: " + re.escape(name) + r"\n", b)]
        assert len(rows) == 1 and "Value: 4\n" in rows[0]
        assert "Section: IMAGE_SYM_UNDEFINED (0)" in rows[0]
        assert re.search(r"^.*\b" + re.escape(name) + r"\s+[0-9a-fA-F]{8}\s+<common>\s*$",
                         maps, re.M), name
    sections = subprocess.check_output(["llvm-readobj", "--sections", args.rebuilt], text=True)
    virtual_zeros = []
    base = export["inputs"]["images"]["rebuilt"]["preferred_base"]
    for section in re.findall(r"  Section \{(.*?)\n  \}", sections, re.S):
        va = int(re.search(r"\n    VirtualAddress: (\S+)", section)[1], 0)
        size = int(re.search(r"\n    VirtualSize: (\S+)", section)[1], 0)
        raw = int(re.search(r"\n    RawDataSize: (\S+)", section)[1], 0)
        if size > raw:
            virtual_zeros.append((base + va + raw, base + va + size))
    assert all(any(first <= slot and slot + 4 <= end for first, end in virtual_zeros)
               for slot in slots), "both COMMON cells must be in the actual PE zero tail"

    writer = export["functions"][WRITER]
    initial = {r["address"]: bytes.fromhex(r["bytes_hex"]) for r in writer["original"]["instructions"]}
    for pc, code in ((0x45394e, "c705107e4900a84b4500"),
                     (0x453a81, "8915107e4900"), (0x4539f2, "c705fc7d4900f3414500"),
                     (0x474b94, "8915fc7d4900"), (0x453927, "ba89eb4100")):
        assert initial[pc] == bytes.fromhex(code)
    writes = {slot: [] for slot in slots}
    for row in writer["rebuilt"]["instructions"]:
        code = bytes.fromhex(row["bytes_hex"])
        if len(code) == 10 and code[:2] == b"\xc7\x05":
            slot, target = struct.unpack("<II", code[2:])
            if slot in writes:
                writes[slot].append(target)
    assert writes[slots[0]] == [address("_output_callback_454ba8"), address("_c2_set_argv")]
    assert writes[slots[1]] == [address("_c2_set_argv"), address("_output_callback_4541f3")]
    source = Path(args.source).read_text()
    begin = source.index("void initialize_pass_functions(void)")
    end = source.index("\nextern int c2_runtime_md", begin)
    assert hashlib.sha256(source[begin:end].encode()).hexdigest() == WRITER_SOURCE_SHA
    for slot in ("497e10", "497dfc"):
        assert "C2OutputFinishCallback output_slot_" + slot + ";" in source
    header = Path(args.header).read_text()
    assert "typedef struct C2OutputFinishCallback { void (__cdecl *target)(void); }" in header
    assert "sizeof(C2OutputFinishCallback) == 4" in header
    assert "offsetof(C2OutputFinishCallback, target) == 0" in header
    return {"owner": OWNER, "other_checked_ret_owners": len(selected) - 1,
            "normal_return_cleanup": 0,
            "scope": "Valid callable normal returns only; NULL remains non-callable, with no default, guard or termination claim. Pending callback bodies remain unaccepted.",
            "basis": "Balanced no-argument CALL, frame-free original tail and complete reviewed RET0 callable alternatives; rebuilt no-argument CALL/CALL plus balanced EBP frame/RET0.",
            "hashes": {name: sha(getattr(args, name.replace("-", "_"))) for name in
                       ("report", "config", "metadata", "ret-checks", "original", "rebuilt",
                        "map", "object", "source", "header")}}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("report", "config", "metadata", "ret-checks", "original", "rebuilt", "map",
                 "object", "source", "header", "output"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    result = check(args)
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n")
    print("PASS: C2 completion normal-return ABI; every other selected RET remains checked")
