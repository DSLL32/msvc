#!/usr/bin/env python3
"""Check immutable raw initial-frontier evidence; never accepts a function."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
INPUTS = ("cases/lifecycle/candidate.json", "functions.json", "runtime.json",
          "graph-metadata.json", "build/c1n32_l.dll", "build/c1xx.map")
TARGET = ROOT / "toolchains/5.10/bin/C1XX.DLL"
CRT = ROOT / "toolchains/5.10/bin/MSVCRT.DLL"
CRT_HASH = "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def checked_receipt(directory):
    receipt = json.loads((directory / "receipt.json").read_text())
    require(receipt["native_sha256"] == sha(PROJECT / "build/native/c1xx-host"),
            "stale native executable")
    require(set(receipt["snapshot_inputs"]) == set(INPUTS), "incomplete snapshot inputs")
    for item in INPUTS:
        frozen = directory / Path(item).name
        require(frozen.read_bytes() == (PROJECT / item).read_bytes(), "stale/wrong trial " + item)
        require(receipt["snapshot_inputs"][item] == sha(frozen), "snapshot hash differs " + item)
    require(receipt["artifacts"], "missing raw artifacts")
    for name, record in receipt["artifacts"].items():
        require(Path(name).name == name, "artifact escaped capture directory")
        path = directory / name
        require(path.is_file() and sha(path) == record["sha256"] and
                path.stat().st_size == record["size"], "raw artifact changed " + name)
    return receipt


def check(original, native):
    o, n = checked_receipt(original), checked_receipt(native)
    require(o["kind"] == "explicit-original-capture" and o["mode"] == "masked",
            "not fresh masked-original evidence")
    require(n["kind"] == "native-gdb-frontier", "not native GDB frontier evidence")
    candidate = json.loads((original / "candidate.json").read_text())
    require(not candidate["accepted"] and candidate["trial"] is None and
            not candidate["ranges"], "checker only covers initial all-masked state")
    expected = candidate["expected_initial_frontier"]
    require(expected["native_symbol"] == "_DllMainCRTStartup" and
            expected["rva"] == 0x2e3c6, "wrong declared initial frontier")
    require(o["target_sha256"] == sha(TARGET) == candidate["target_sha256"] ==
            expected["module_sha256"], "wrong original module")
    require(sha(original / "loaded-target.bin") == sha(TARGET), "wrong actual loaded target")
    require(sha(original / "loaded-msvcrt.bin") == o["canonical_crt_sha256"] ==
            sha(CRT) == CRT_HASH, "wrong actual loaded CRT provider")
    require(o["exit"] == 125, "original did not stop at a frontier")
    text = (original / "events.txt").read_text()
    frontiers = re.findall(r"^frontier rva=([0-9a-f]{8})$", text, re.M)
    traps = re.findall(r"^trap seq=0 rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$", text, re.M)
    modules = re.findall(r"^module name=c1n32_l.dll actual=([0-9a-f]{8}) preferred=([0-9a-f]{8}) entry=([0-9a-f]{8})$", text, re.M)
    require(len(frontiers) == len(traps) == len(modules) == 1, "missing/ambiguous raw frontier or module")
    require(int(frontiers[0], 16) == int(traps[0][0], 16) == expected["rva"],
            "raw original frontier differs")
    require("failure " not in text and "mask snapshot=relocated-text.bin baseline=0 ranges=0" in text,
            "original masking failed")
    snapshot = original / "relocated-text.bin"
    require(snapshot.stat().st_size == 0xb7670 and
            snapshot.read_bytes()[0x2e3c6 - 0x1000] == 0x53, "missing original relocated snapshot")
    require((original / "state-000.bin").stat().st_size == 0xa4,
            "missing initial raw target state")
    stack = [int(word, 16) for word in traps[0][1].split(",")]
    require(stack[1:4] == [int(modules[0][0], 16), 1, 0], "wrong raw original lifecycle arguments")
    state = json.loads((native / "state.json").read_text())
    require(state["symbol"] == expected["native_symbol"], "native raw function differs")
    transcript = (native / "gdb.txt").read_text()
    require("Program received signal SIGTRAP" in transcript and
            "in " + state["symbol"] + " (" in transcript, "native transcript has no target trap")
    require(state["registration"] == {
        "name": "C1XX.DLL", "path": "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
        "sha256": sha(TARGET), "flags": 2}, "wrong native module registration provenance")
    require(state["args"]["module"] == state["registered_token"] != 0 and
            [state["args"]["reason"], state["args"]["reserved"]] == stack[2:4],
            "native module identity/lifecycle arguments differ")
    binary = PROJECT / "build/native/c1xx-host"
    symbols = subprocess.check_output(["nm", "-S", "--defined-only", str(binary)], text=True)
    match = re.search(r"^([0-9a-f]+) ([0-9a-f]+) [Tt] " + re.escape(state["symbol"]) + "$", symbols, re.M)
    require(match is not None, "native frontier is not a real ELF function")
    start, size = (int(word, 16) for word in match.groups())
    require(start < state["pc"] <= start + size, "native trap PC outside function")
    opcode = subprocess.check_output(["objdump", "-d", "--start-address=" + str(state["pc"] - 1),
                                      "--stop-address=" + str(state["pc"]), str(binary)], text=True)
    require("int3" in opcode, "native PC is not immediately after INT3")
    return {"status": "fresh initial matching frontier", "rva": expected["rva"],
            "symbol": state["symbol"], "accepted_functions": 0,
            "scope": "DLL entry plus three ABI arguments and module identity; not lifecycle/function acceptance"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    print(json.dumps(check(args.original, args.native), indent=2))
