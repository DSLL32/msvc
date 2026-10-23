#!/usr/bin/env python3
"""Exercise stale-trial and raw-frontier rejection without running targets."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile

spec = importlib.util.spec_from_file_location("pair", Path(__file__).with_name("check-pair.py"))
pair = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pair)


def rehash(directory, name):
    path = directory / name
    receipt_path = directory / "receipt.json"
    receipt = json.loads(receipt_path.read_text())
    receipt["artifacts"][name] = {"sha256": pair.sha(path), "size": path.stat().st_size}
    receipt_path.write_text(json.dumps(receipt))


def edit_json(path, change):
    value = json.loads(path.read_text())
    change(value)
    path.write_text(json.dumps(value))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    pair.check(args.original, args.native)
    scratch = Path.home() / ".ai-tmp/msvc2/vc5.10/c1xx"
    cases = ("stale-native", "stale-rebuilt", "wrong-trial", "missing-raw-frontier",
             "wrong-raw-rva", "missing-gdb-trap", "wrong-native-pc",
             "missing-native-symbol", "wrong-registration", "wrong-module-argument",
             "wrong-loaded-snapshot", "false-completion")
    for case in cases:
        with tempfile.TemporaryDirectory(prefix="pair-negative-", dir=scratch) as temp:
            original, native = Path(temp) / "original", Path(temp) / "native"
            shutil.copytree(args.original, original)
            shutil.copytree(args.native, native)
            if case == "stale-native":
                for directory in (original, native):
                    edit_json(directory / "receipt.json", lambda j: j.update(native_sha256="0" * 64))
            elif case == "stale-rebuilt":
                for directory in (original, native):
                    edit_json(directory / "receipt.json", lambda j: j["snapshot_inputs"].update({"build/c1n32_l.dll": "0" * 64}))
            elif case == "wrong-trial":
                for directory in (original, native):
                    edit_json(directory / "candidate.json", lambda j: j.update(trial="_DllMain@12"))
                    rehash(directory, "candidate.json")
                    edit_json(directory / "receipt.json", lambda j: j["snapshot_inputs"].update({"cases/lifecycle/candidate.json": pair.sha(directory / "candidate.json")}))
            elif case in ("missing-raw-frontier", "wrong-raw-rva", "false-completion"):
                path = original / "events.txt"
                text = path.read_text()
                if case in ("missing-raw-frontier", "false-completion"):
                    text = "\n".join(line for line in text.splitlines() if not line.startswith("frontier ")) + "\n"
                else:
                    text = text.replace("frontier rva=0002e3c6", "frontier rva=0002e52a")
                path.write_text(text)
                rehash(original, path.name)
                if case == "false-completion":
                    edit_json(original / "receipt.json", lambda j: j.update(exit=0))
            elif case == "missing-gdb-trap":
                path = native / "gdb.txt"
                path.write_text(path.read_text().replace("Program received signal SIGTRAP", "No signal"))
                rehash(native, path.name)
            elif case in ("wrong-native-pc", "missing-native-symbol", "wrong-registration", "wrong-module-argument"):
                if case == "wrong-native-pc":
                    change = lambda j: j.update(pc=1)
                elif case == "missing-native-symbol":
                    change = lambda j: j.update(symbol=None)
                elif case == "wrong-registration":
                    change = lambda j: j["registration"].update(sha256="0" * 64)
                else:
                    change = lambda j: j["args"].update(module=1)
                edit_json(native / "state.json", change)
                rehash(native, "state.json")
            else:
                path = original / "relocated-text.bin"
                raw = bytearray(path.read_bytes())
                raw[0x2e3c6 - 0x1000] = 0xcc
                path.write_bytes(raw)
                rehash(original, path.name)
            try:
                pair.check(original, native)
            except (ValueError, KeyError, TypeError):
                continue
            raise RuntimeError("accepted invalid evidence: " + case)
    print(json.dumps({"count": len(cases), "rejected": list(cases)}))


if __name__ == "__main__":
    main()
