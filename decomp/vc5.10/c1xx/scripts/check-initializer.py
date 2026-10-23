"""Typed CRT_INIT-to-_initterm import-boundary decoder."""
import importlib.util
import json
from pathlib import Path
import re
import subprocess

spec = importlib.util.spec_from_file_location("support", Path(__file__).with_name("trial-support.py"))
s = importlib.util.module_from_spec(spec)
spec.loader.exec_module(s)


def check(original, native, require_audit=True, audit_path=None):
    o, n, trial, text, state = s.pair(original, native)
    s.require(trial["trial"] == "__CRT_INIT@12" and o["exit"] == 125,
              "not the CRT_INIT frontier trial")
    frontiers = re.findall(r"^frontier rva=([0-9a-f]{8})$", text, re.M)
    traps = re.findall(r"^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)$", text, re.M)
    s.require(frontiers == ["000384ce"] and [row[:2] for row in traps] ==
              [("0", "0002e3c6"), ("1", "0002e46a"), ("2", "000384ce")], "raw import frontier differs")
    old = s.original_state(original, text, 2)
    current = state["target_state"]
    for values in (old, current):
        s.require(values["process_attached"] == values["raw_dll_entry"] == values["dll_adjust_fdiv"] == 0,
                  "typed CRT scalars differ before initializer callback")
        s.require(values["initializer_words"] == [0, 0], "initializer table/sentinel differs")
        s.require(values["onexit_begin"] != 0 and values["onexit_end"] == values["onexit_begin"] and
                  values["onexit_first"] == 0, "initialized onexit allocation/empty-end identity differs")
    stack = [int(word, 16) for word in traps[-1][2].split(",")]
    s.require(stack[1:3] == [0x104cb010, 0x104cb014], "original initializer range differs")
    s.require(state["symbol"] == "recon_initterm" and state["args"] == {
        "begin": current["initializer_begin"], "end": current["initializer_begin"] + 4} and
        state["raw_stack"][1:3] == [state["args"]["begin"], state["args"]["end"]],
        "native cdecl initializer pointers/exclusive end differ")
    providers = re.findall(r"^provider iat=([0-9a-f]{8}) resolved=([0-9a-f]{8})$", text, re.M)
    s.require(providers == [("000c9114", "78001715"), ("000c90a8", "78001498"),
                            ("000c9140", "7800138a"), ("000c9118", "7803f928")], "canonical import identities differ")
    s.require(re.findall(r"^provider-adjust-fdiv=(\d+)$", text, re.M) == ["0"] and
              current["provider_adjust_fdiv"] == 0, "imported CRT data differs")
    last_errors = re.findall(r"^last-error=(\d+)$", text, re.M)
    s.require(last_errors == ["203", "203", "203"] and current["last_error"] == 203,
              "captured Win32 loader context/LastError changed")
    transcript = (native / "gdb.txt").read_text()
    s.require("Breakpoint 1, recon_initterm" in transcript and
              "break *recon_initterm" in (native / "commands.gdb").read_text(), "missing native provider-entry stop")
    symbols = subprocess.check_output(["nm", "-S", "--defined-only", str(s.PROJECT / "build/native/c1xx-host")], text=True)
    match = re.search(r"^([0-9a-f]+) ([0-9a-f]+) [TWtw] recon_initterm$", symbols, re.M)
    s.require(match is not None and state["pc"] == int(match[1], 16), "native provider stop is not actual ELF entry")
    if require_audit:
        s.audit(trial, audit_path)
    return {"trial": trial["trial"], "frontier": "import:_initterm", "raw_frontier_rva": 0x384ce,
            "typed_state": {"attach_count": 0, "raw_callback": "NULL", "onexit_allocation_bytes": 128,
                            "initialized_heap_bytes_compared": 4, "onexit_first": 0, "onexit_end": "allocation+0",
                            "initializer_slots": [None], "exclusive_end": "table+4", "adjust_fdiv": 0,
                            "last_error": 203}, "whole_mask_bytes_checked": 751216,
            "original_reused": o["native_sha256"] != n["native_sha256"],
            "original_trial_sha256": s.sha(original / "trial.json"),
            "current_trial_sha256": s.sha(native / "trial.json"),
            "audit_checked": require_audit, "durably_accepted": len(trial["accepted"]) - 1}
