#!/usr/bin/env python3
"""Reject altered initializer heap, callback, import and reused-input evidence."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import tempfile

spec = importlib.util.spec_from_file_location("decoder", Path(__file__).with_name("check-initializer.py"))
d = importlib.util.module_from_spec(spec)
spec.loader.exec_module(d)


def edit(path, change):
    value = json.loads(path.read_text())
    change(value)
    path.write_text(json.dumps(value))


def rehash(directory, name):
    path = directory / name
    edit(directory / "receipt.json", lambda j: j["artifacts"].update({
        name: {"sha256": d.s.sha(path), "size": path.stat().st_size}}))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("original", type=Path)
    p.add_argument("native", type=Path)
    args = p.parse_args()
    d.check(args.original, args.native)
    cases = ("stale-native", "bad-old-native-binding", "changed-reused-mask", "changed-reused-case",
             "wrong-heap-first", "wrong-empty-heap-end", "nonnull-initializer-slot", "wrong-exclusive-end",
             "wrong-provider-value", "wrong-last-error", "wrong-provider-entry", "original-pointer-in-native",
             "changed-compiled-source-content", "missing-cold-audit")
    scratch = Path.home() / ".ai-tmp/msvc2/vc5.10/c1xx"
    for case in cases:
        with tempfile.TemporaryDirectory(prefix="initializer-negative-", dir=scratch) as temp:
            root = Path(temp)
            original, native = root / "original", root / "native"
            shutil.copytree(args.original, original)
            shutil.copytree(args.native, native)
            audit = root / "audit.json"
            shutil.copy2(d.s.PROJECT / "build/audit-proof.json", audit)
            if case in ("stale-native", "bad-old-native-binding"):
                directory = native if case == "stale-native" else original
                edit(directory / "receipt.json", lambda j: j.update(native_sha256="0" * 64))
            elif case in ("changed-reused-mask", "changed-reused-case"):
                change = (lambda j: j["ranges"].pop()) if case.endswith("mask") else (lambda j: j.update(case="other"))
                edit(original / "trial.json", change)
                rehash(original, "trial.json")
                edit(original / "receipt.json", lambda j: j["snapshot_inputs"].update({"build/trial.json": d.s.sha(original / "trial.json")}))
            elif case == "wrong-heap-first":
                path = original / "events.txt"
                text = path.read_text()
                lines = [line.replace("heap-first=00000000", "heap-first=00000001")
                         if line.startswith("state seq=2 ") else line for line in text.splitlines()]
                path.write_text("\n".join(lines) + "\n")
                rehash(original, path.name)
            elif case in ("wrong-empty-heap-end", "nonnull-initializer-slot"):
                path = original / "state-002.bin"
                raw = bytearray(path.read_bytes())
                if case == "wrong-empty-heap-end":
                    struct.pack_into("<I", raw, 0x20, struct.unpack_from("<I", raw, 0x1c)[0] + 4)
                else:
                    struct.pack_into("<I", raw, 0x10, 1)
                path.write_bytes(raw)
                rehash(original, path.name)
            elif case in ("wrong-exclusive-end", "wrong-provider-value", "wrong-last-error", "wrong-provider-entry", "original-pointer-in-native"):
                def change(j):
                    if case == "wrong-exclusive-end":
                        j["args"]["end"] += 4
                        j["raw_stack"][2] += 4
                    elif case == "wrong-provider-value":
                        j["target_state"]["provider_adjust_fdiv"] = 1
                    elif case == "wrong-last-error":
                        j["target_state"]["last_error"] = 204
                    elif case == "wrong-provider-entry":
                        j["pc"] += 1
                    else:
                        j["args"] = {"begin": 0x104cb010, "end": 0x104cb014}
                        j["raw_stack"][1:3] = [0x104cb010, 0x104cb014]
                edit(native / "state.json", change)
                rehash(native, "state.json")
            elif case == "changed-compiled-source-content":
                path = next((native / "source-content").iterdir())
                path.write_bytes(path.read_bytes() + b"\n")
            else:
                edit(audit, lambda j: j["report"]["owned_code_chunks"]["original"]["functions"].update({
                    "__CRT_INIT@12": [[0x1042e46a, 0x1042e52a]]}))
            try:
                d.check(original, native, audit_path=audit)
            except (ValueError, KeyError, TypeError):
                continue
            raise RuntimeError("accepted invalid initializer proof " + case)
    print(json.dumps({"count": len(cases), "rejected": list(cases)}))


if __name__ == "__main__":
    main()
