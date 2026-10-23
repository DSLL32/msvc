#!/usr/bin/env python3
"""Exercise first-trial binding, real-frontier and typed-state rejection checks."""
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


def rehash(directory, name):
    receipt_path = directory / "receipt.json"
    receipt = json.loads(receipt_path.read_text())
    path = directory / name
    receipt["artifacts"][name] = {"sha256": trial.pair.sha(path), "size": path.stat().st_size}
    receipt_path.write_text(json.dumps(receipt))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    trial.check(args.original, args.native)
    names = ("stale-native", "stale-rebuilt", "wrong-trial", "no-frontier",
             "false-frontier", "false-pc", "wrong-module-token", "wrong-module-hash",
             "wrong-reason", "wrong-attach-count", "nonnull-raw-hook", "wrong-last-error",
             "changed-original-state", "prefix-only-audit", "missing-cold-chunk", "old-auditor-report")
    scratch = Path.home() / ".ai-tmp/msvc2/vc5.10/c1"
    for name in names:
        with tempfile.TemporaryDirectory(prefix="trial-negative-", dir=scratch) as temp:
            original, native = Path(temp) / "original", Path(temp) / "native"
            audit = Path(temp) / "audit"
            shutil.copytree(args.original, original)
            shutil.copytree(args.native, native)
            audit.mkdir()
            for filename in ("trial-audit.json", "trial-audit-binding.json"):
                shutil.copy2(trial.PROJECT / "build" / filename, audit / filename)
            if name.startswith("stale-"):
                key = "native_sha256" if name == "stale-native" else "rebuilt_sha256"
                for directory in (original, native):
                    path = directory / "receipt.json"
                    value = json.loads(path.read_text()); value[key] = "0" * 64
                    path.write_text(json.dumps(value))
            elif name == "wrong-trial":
                for directory in (original, native):
                    path = directory / "candidate.json"
                    value = json.loads(path.read_text()); value["functions"] = []
                    path.write_text(json.dumps(value)); rehash(directory, path.name)
                    path = directory / "receipt.json"
                    value = json.loads(path.read_text()); value["candidate_sha256"] = trial.pair.sha(directory / "candidate.json")
                    path.write_text(json.dumps(value))
            elif name in ("prefix-only-audit", "missing-cold-chunk", "old-auditor-report"):
                path = audit / "trial-audit.json"
                value = json.loads(path.read_text())
                if name == "prefix-only-audit":
                    value["checked_functions"] = 0
                    value["functions"] = {}
                elif name == "missing-cold-chunk":
                    value["owned_code_chunks"]["original"]["functions"]["_c1_crt_entry@12"].pop()
                else:
                    del value["owned_code_chunks"]
                path.write_text(json.dumps(value))
                binding_path = audit / "trial-audit-binding.json"
                binding = json.loads(binding_path.read_text())
                binding["report_sha256"] = trial.pair.sha(path)
                binding_path.write_text(json.dumps(binding))
            elif name in ("no-frontier", "false-frontier"):
                path = original / "events.txt"
                value = path.read_text()
                value = ("\n".join(line for line in value.splitlines() if not line.startswith("frontier ")) + "\n"
                         if name == "no-frontier" else value.replace("frontier rva=000120ce", "frontier rva=0001218e"))
                path.write_text(value); rehash(original, path.name)
            elif name == "changed-original-state":
                path = original / "state-001.bin"
                value = bytearray(path.read_bytes()); struct.pack_into("<i", value, 0x90, 1)
                path.write_bytes(value); rehash(original, path.name)
            else:
                path = native / "state.json"
                value = json.loads(path.read_text())
                if name == "false-pc": value["pc"] = 1
                elif name == "wrong-module-token": value["args"]["module"] = 0x10600000
                elif name == "wrong-module-hash": value["module_identity"]["sha256"] = "0" * 64
                elif name == "wrong-reason": value["args"]["reason"] = 2
                elif name == "wrong-attach-count": value["crt_attached"] = 1
                elif name == "nonnull-raw-hook": value["raw_dllmain"] = 1
                elif name == "wrong-last-error": value["win32_last_error"] = 0
                path.write_text(json.dumps(value)); rehash(native, path.name)
            try:
                trial.check(original, native, audit)
            except (AssertionError, KeyError, ValueError):
                continue
            raise AssertionError("accepted negative " + name)
    print(json.dumps({"count": len(names), "rejected": list(names)}))


if __name__ == "__main__":
    main()
