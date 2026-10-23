#!/usr/bin/env python3
"""Exercise first-trial binding, real-frontier and typed-state rejection checks."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile
import re

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
    candidate = json.loads((args.original / "candidate.json").read_text())
    import_boundary = candidate["expected_frontier"].get("kind") == "import-boundary"
    names = ("stale-native", "stale-rebuilt", "wrong-trial", "no-frontier",
             "false-frontier", "false-pc", "wrong-module-token", "wrong-module-hash",
             "wrong-reason", "wrong-attach-count", "nonnull-raw-hook", "wrong-last-error",
             "wrong-x87-control", "changed-original-state", "prefix-only-audit", "missing-cold-chunk", "old-auditor-report")
    if import_boundary:
        names += ("initializer-range", "initializer-slot", "onexit-alias", "onexit-slot", "adjust-flag", "premature-module")
    if (args.native / "source-snapshot.json").exists():
        names += ("frozen-source-content",)
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
            elif name == "frozen-source-content":
                path = native / "source-snapshot.json"
                value = json.loads(path.read_text())
                forged = Path(temp) / "changed-source.txt"
                forged.write_text("changed after capture\n")
                value[next(iter(value))]["content"] = str(forged)
                path.write_text(json.dumps(value)); rehash(native, path.name)
            elif name in ("no-frontier", "false-frontier"):
                path = original / "events.txt"
                value = path.read_text()
                value = ("\n".join(line for line in value.splitlines() if not line.startswith("frontier ")) + "\n"
                         if name == "no-frontier" else re.sub(r"^frontier rva=[0-9a-f]+$", "frontier rva=deadbeef", value, flags=re.M))
                path.write_text(value); rehash(original, path.name)
            elif name == "changed-original-state":
                rows = re.findall(r"^trap seq=(\d+)", (original / "events.txt").read_text(), re.M)
                path = original / ("state-%03d.bin" % int(rows[-1]))
                value = bytearray(path.read_bytes()); struct.pack_into("<i", value, 0x90, struct.unpack_from("<i", value, 0x90)[0] ^ 1)
                path.write_bytes(value); rehash(original, path.name)
            else:
                path = native / "state.json"
                value = json.loads(path.read_text())
                if name == "false-pc": value["pc"] = 1
                elif name == "wrong-module-token":
                    if import_boundary: value["caller"]["args"]["module"] = 0x10600000
                    else: value["args"]["module"] = 0x10600000
                elif name == "wrong-module-hash": value["module_identity"]["sha256"] = "0" * 64
                elif name == "wrong-reason":
                    if import_boundary: value["caller"]["args"]["reason"] = 2
                    else: value["args"]["reason"] = 2
                elif name == "wrong-attach-count": value["crt_attached"] ^= 1
                elif name == "nonnull-raw-hook": value["raw_dllmain"] = 1
                elif name == "wrong-x87-control": value["x87_control"] = 0x037f
                elif name == "wrong-last-error": value["win32_last_error"] = 0
                elif name == "initializer-range": value["args"]["end"] += 4
                elif name == "initializer-slot": value["initializer_slots"] = [1]
                elif name == "onexit-alias": value["onexit_end"] += 4
                elif name == "onexit-slot": value["onexit_first"] = 1
                elif name == "adjust-flag": value["crt_adjust_fdiv"] = 1
                elif name == "premature-module": value["dll_instance"] = value["module_token"]
                path.write_text(json.dumps(value)); rehash(native, path.name)
            try:
                trial.check(original, native, audit)
            except (AssertionError, KeyError, ValueError):
                continue
            raise AssertionError("accepted negative " + name)
    print(json.dumps({"count": len(names), "rejected": list(names)}))


if __name__ == "__main__":
    main()
