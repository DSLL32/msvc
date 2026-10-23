#!/usr/bin/env python3
"""Reject stale trials, altered raw state/masks and incomplete candidate audits."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile

spec = importlib.util.spec_from_file_location("trial", Path(__file__).with_name("check-trial.py"))
trial = importlib.util.module_from_spec(spec)
spec.loader.exec_module(trial)


def edit_json(path, change):
    value = json.loads(path.read_text())
    change(value)
    path.write_text(json.dumps(value))


def rehash(directory, name):
    path = directory / name
    edit_json(directory / "receipt.json", lambda j: j["artifacts"].update({
        name: {"sha256": trial.sha(path), "size": path.stat().st_size}}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    trial.check(args.original, args.native)
    scratch = Path.home() / ".ai-tmp/msvc2/vc5.10/c1xx"
    cases = ("stale-native", "stale-rebuilt", "wrong-trial", "missing-frontier",
             "wrong-original-rva", "false-completion", "missing-native-trap",
             "missing-native-symbol", "wrong-native-pc", "wrong-module-token",
             "wrong-registration", "wrong-native-attach-count", "wrong-original-callback",
             "cold-body-not-restored", "unaccepted-code-restored", "stale-auditor",
             "candidate-omitted-from-audit", "cold-body-omitted-from-audit", "stale-audited-trial")
    for case in cases:
        with tempfile.TemporaryDirectory(prefix="trial-negative-", dir=scratch) as temp:
            directory = Path(temp)
            original, native = directory / "original", directory / "native"
            shutil.copytree(args.original, original)
            shutil.copytree(args.native, native)
            audit = directory / "audit.json"
            shutil.copy2(trial.PROJECT / "build/audit-proof.json", audit)
            if case == "stale-native":
                for capture in (original, native):
                    edit_json(capture / "receipt.json", lambda j: j.update(native_sha256="0" * 64))
            elif case == "stale-rebuilt":
                for capture in (original, native):
                    edit_json(capture / "receipt.json", lambda j: j["snapshot_inputs"].update({"build/c1n32_l.dll": "0" * 64}))
            elif case == "wrong-trial":
                for capture in (original, native):
                    edit_json(capture / "trial.json", lambda j: j.update(trial="__CRT_INIT@12"))
                    rehash(capture, "trial.json")
                    edit_json(capture / "receipt.json", lambda j: j["snapshot_inputs"].update({"build/trial.json": trial.sha(capture / "trial.json")}))
            elif case in ("missing-frontier", "wrong-original-rva", "false-completion"):
                path = original / "events.txt"
                text = path.read_text()
                if case == "wrong-original-rva":
                    text = text.replace("frontier rva=0002e46a", "frontier rva=0002e52a")
                else:
                    text = "\n".join(line for line in text.splitlines() if not line.startswith("frontier ")) + "\n"
                path.write_text(text)
                rehash(original, path.name)
                if case == "false-completion":
                    edit_json(original / "receipt.json", lambda j: j.update(exit=0))
            elif case == "missing-native-trap":
                path = native / "gdb.txt"
                path.write_text(path.read_text().replace("Program received signal SIGTRAP", "No signal"))
                rehash(native, path.name)
            elif case in ("missing-native-symbol", "wrong-native-pc", "wrong-module-token", "wrong-registration", "wrong-native-attach-count"):
                changes = {
                    "missing-native-symbol": lambda j: j.update(symbol=None),
                    "wrong-native-pc": lambda j: j.update(pc=1),
                    "wrong-module-token": lambda j: j["args"].update(module=1),
                    "wrong-registration": lambda j: j["registration"].update(sha256="0" * 64),
                    "wrong-native-attach-count": lambda j: j["target_state"].update(process_attached=7),
                }
                edit_json(native / "state.json", changes[case])
                rehash(native, "state.json")
            elif case == "wrong-original-callback":
                path = original / "state-001.bin"
                raw = bytearray(path.read_bytes())
                struct.pack_into("<I", raw, 0x24, 1)
                path.write_bytes(raw)
                rehash(original, path.name)
            elif case in ("cold-body-not-restored", "unaccepted-code-restored"):
                path = original / "masked-text.bin"
                raw = bytearray(path.read_bytes())
                if case == "cold-body-not-restored":
                    raw[0x6acb3 - 0x1000] = 0xcc
                else:
                    raw[0] = (original / "relocated-text.bin").read_bytes()[0]
                path.write_bytes(raw)
                rehash(original, path.name)
            elif case == "stale-auditor":
                edit_json(audit, lambda j: j.update(auditor_sha256="0" * 64))
            elif case == "candidate-omitted-from-audit":
                edit_json(audit, lambda j: j["report"].update(checked_functions=0, functions={}))
            elif case == "cold-body-omitted-from-audit":
                edit_json(audit, lambda j: j["report"]["owned_code_chunks"]["original"]["functions"].update({
                    "__DllMainCRTStartup@12": [[0x1042e3c6, 0x1042e46a]]}))
            else:
                edit_json(audit, lambda j: j.update(trial_sha256="0" * 64))
            try:
                trial.check(original, native, audit_path=audit)
            except (ValueError, KeyError, TypeError):
                continue
            raise RuntimeError("accepted invalid candidate proof: " + case)
    print(json.dumps({"count": len(cases), "rejected": list(cases)}))


if __name__ == "__main__":
    main()
