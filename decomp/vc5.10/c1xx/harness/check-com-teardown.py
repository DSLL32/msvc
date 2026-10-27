#!/usr/bin/env python3
"""Close only destroy_generated_comment's split CoUninitialize tail ABI.

Shared graph-audit checks every owner and every other RET. Its hash-bound
instruction export and import catalogs supply this finite residual; this
checker does not discover functions, parse PE files or execute COM.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct


OWNER = "_destroy_generated_comment"
IMPORT = {"dll": "ole32.dll", "name": "CoUninitialize"}
ORIGINAL_SHA = "e27df3bb9a37058c85dad3fb7d1ed30b9e52d38fb6c0f66c8fdba0a11111d3d9"
SDK_SHA = "5961de8d21a897b360036706fcc6f07b94e4ca0fbe4690b10c24cf5d5e0de9cc"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def semantic_sha(path):
    value = json.loads(Path(path).read_text())
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()
    return hashlib.sha256(encoded).hexdigest()


def check(args):
    report = json.loads(Path(args.report).read_text())
    export = report["decoded_instructions"]
    inputs = export["inputs"]
    assert export["schema"] == 1
    for key, path in (("config_sha256", args.config), ("metadata_sha256", args.metadata)):
        assert inputs[key] == semantic_sha(path), key
    images = {}
    for side, path in (("original", args.original), ("rebuilt", args.rebuilt)):
        assert inputs["images"][side]["format"] == "pe32"
        assert inputs["images"][side]["original_file_sha256"] == sha(path), side
        images[side] = Path(path).read_bytes()
    assert sha(args.original) == ORIGINAL_SHA
    selected = set(json.loads(Path(args.config).read_text())["accepted"])
    contracts = json.loads(Path(args.ret_checks).read_text())
    assert contracts["schema"] == 1
    assert set(contracts["functions"]) == selected - {OWNER}
    assert set(export["functions"]) == selected
    checks = report["ret_cleanup_checks"]
    assert checks["input_sha256"] == sha(args.ret_checks)
    assert set(checks["functions"]) == selected - {OWNER}
    for name, contract in contracts["functions"].items():
        expected = contract["expected_cleanup"]
        assert expected
        for side in ("original", "rebuilt"):
            assert checks["functions"][name][side] == expected
            assert export["functions"][name][side]["checked_ret_cleanup_set"] == expected

    function = export["functions"][OWNER]
    rows = {}
    for side in ("original", "rebuilt"):
        body = function[side]
        assert body["local_ret_cleanup_set"] == [0]
        assert body["checked_ret_cleanup_set"] is None
        assert not body["declared_roots"] and not body["declared_text_data"]
        rows[side] = [(i["address"], bytes.fromhex(i["bytes_hex"]))
                      for i in body["instructions"]]
    original = [
        (0x104346D4, bytes.fromhex("a1646b4c10")),
        (0x104346D9, bytes.fromhex("85c0")),
        (0x104346DB, bytes.fromhex("0f85aeab0300")),
        (0x104346E1, bytes.fromhex("c3")),
        (0x1046F28F, bytes.fromhex("c705646b4c1000000000")),
    ]
    assert rows["original"] == original
    record = function["original"]["record"]
    assert (record["start"], record["end"]) == (0x104346D4, 0x104346E2)
    assert record["code_chunks"] == [[0x1046F28F, 0x1046F299]]
    # Exact pristine bytes establish the excluded FF25 at the cold boundary;
    # the shared, image-bound import catalog independently identifies its slot.
    slot = 0x104C924C
    thunk = b"\xff\x25" + struct.pack("<I", slot)
    assert images["original"].count(original[-1][1] + thunk) == 1
    assert report["import_catalogs"]["original"]["slots"][str(slot)] == IMPORT
    assert images["original"].count(b"".join(b for _, b in original[:4])) == 1

    rebuilt = rows["rebuilt"]
    assert len(rebuilt) == 8
    pc = function["rebuilt"]["record"]["start"]
    for address, code in rebuilt:
        assert address == pc
        pc += len(code)
    assert pc == function["rebuilt"]["record"]["end"]
    code = [b for _, b in rebuilt]
    assert code[:2] == [b"\x55", b"\x8b\xec"]
    assert len(code[2]) == 7 and code[2][:2] == b"\x83\x3d" and code[2][-1:] == b"\0"
    flag = code[2][2:6]
    assert code[3] == b"\x74\x10"
    assert code[4] == b"\xc7\x05" + flag + bytes(4)
    assert len(code[5]) == 6 and code[5][:2] == b"\xff\x15"
    slot = struct.unpack("<I", code[5][2:])[0]
    assert report["import_catalogs"]["rebuilt"]["slots"][str(slot)] == IMPORT
    assert code[6:] == [b"\x5d", b"\xc3"]
    assert images["rebuilt"].count(b"".join(code)) == 1
    assert report["functions"][OWNER]["imports"] == [IMPORT]
    assert report["functions"][OWNER]["edges"] == []

    sdk = Path(args.sdk).read_bytes()
    assert sha(args.sdk) == SDK_SHA
    for declaration in (b"WINOLEAPI_(void)  CoUninitialize(void);",
                        b"#define STDAPICALLTYPE          __stdcall",
                        b"#define WINOLEAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE"):
        assert declaration in sdk
    return {"owner": OWNER, "other_checked_ret_owners": len(contracts["functions"]),
            "cleanup": 0, "basis": "SDK void WINAPI(void): stdcall0 equals both local RET0 paths",
            "original_sha256": sha(args.original), "rebuilt_sha256": sha(args.rebuilt),
            "sdk_sha256": SDK_SHA, "report_sha256": sha(args.report),
            "scope": "Static guard/clear/import/local-RET equivalence only; current flag0 skips the COM call."}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("report", "config", "metadata", "ret-checks", "original", "rebuilt", "sdk", "output"):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    result = check(args)
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n")
    print("PASS: all selected RET owners plus the finite COM stdcall0 boundary")
