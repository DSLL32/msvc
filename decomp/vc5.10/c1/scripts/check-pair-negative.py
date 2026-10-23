#!/usr/bin/env python3
"""Reject stale, mismatched and raw-missing initial captures; no execution."""
import argparse
import hashlib
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
    receipt["artifacts"][name] = {"sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                                  "size": path.stat().st_size}
    receipt_path.write_text(json.dumps(receipt))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    pair.check(args.original, args.native)
    root = Path.home() / ".ai-tmp/msvc2/vc5.10/c1"
    cases = ("stale-native", "stale-rebuilt", "wrong-trial", "no-raw-frontier",
             "wrong-raw-rva", "missing-gdb-trap", "wrong-native-pc")
    for case in cases:
        with tempfile.TemporaryDirectory(prefix="pair-negative-", dir=root) as temp:
            original, native = Path(temp) / "original", Path(temp) / "native"
            shutil.copytree(args.original, original)
            shutil.copytree(args.native, native)
            if case in ("stale-native", "stale-rebuilt"):
                key = "native_sha256" if case == "stale-native" else "rebuilt_sha256"
                for directory in (original, native):
                    receipt_path = directory / "receipt.json"
                    value = json.loads(receipt_path.read_text())
                    value[key] = "0" * 64
                    receipt_path.write_text(json.dumps(value))
            elif case == "wrong-trial":
                for directory in (original, native):
                    path = directory / "candidate.json"
                    value = json.loads(path.read_text())
                    value["ranges"] = [{"start_rva": 0x1202a, "end_rva": 0x120ce, "entry": 1}]
                    path.write_text(json.dumps(value))
                    rehash(directory, path.name)
                    receipt_path = directory / "receipt.json"
                    receipt = json.loads(receipt_path.read_text())
                    receipt["candidate_sha256"] = pair.sha(path)
                    receipt_path.write_text(json.dumps(receipt))
            elif case in ("no-raw-frontier", "wrong-raw-rva"):
                path = original / "events.txt"
                value = path.read_text()
                if case == "no-raw-frontier":
                    value = "\n".join(line for line in value.splitlines() if not line.startswith("frontier ")) + "\n"
                else:
                    value = value.replace("frontier rva=0001202a", "frontier rva=0001218e")
                path.write_text(value)
                rehash(original, path.name)
            elif case == "missing-gdb-trap":
                path = native / "gdb.txt"
                path.write_text(path.read_text().replace("Program received signal SIGTRAP", "No signal"))
                rehash(native, path.name)
            elif case == "wrong-native-pc":
                path = native / "state.json"
                value = json.loads(path.read_text())
                value["pc"] = 1
                path.write_text(json.dumps(value))
                rehash(native, path.name)
            try:
                pair.check(original, native)
            except (AssertionError, KeyError, ValueError):
                continue
            raise AssertionError("accepted negative " + case)
    print(json.dumps({"rejected": list(cases), "count": len(cases)}))


if __name__ == "__main__":
    main()
