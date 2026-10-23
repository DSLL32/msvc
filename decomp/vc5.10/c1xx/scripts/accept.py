#!/usr/bin/env python3
"""Promote a fully checked trial; reconstruction policy approval is external."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("trial_check", Path(__file__).with_name("check-trial.py"))
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    result = checker.check(args.original, args.native)
    trial = json.loads((args.original / "trial.json").read_text())
    cfg = json.loads((PROJECT / "functions.json").read_text())
    candidate_path = cfg.get("candidate_path", "cases/lifecycle/candidate.json")
    candidate = json.loads((PROJECT / candidate_path).read_text())
    if trial["trial"] != candidate["trial"] or trial["trial"] in cfg["accepted"]:
        raise ValueError("trial is not exactly one new function")
    cfg["accepted"] = trial["accepted"]
    cfg["status"] = "autonomous minimal C++ reconstruction active" if trial["case"] == "minimal" else "lifecycle reconstruction active"
    source = "src/dllmain.c" if trial["trial"] == "_DllMain@12" else "src/startup.c"
    classification = "application source" if trial["trial"] == "_DllMain@12" else "target CRT support source"
    if trial["trial"].startswith("import:"):
        source = "src/c1xx.h"
        classification = "linker-generated import thunk; ordinary external declaration"
    if trial["case"] == "minimal":
        source = next(row["source"] for row in cfg["driver"] if row["name"] == trial["trial"])
        classification = "application source"
    cfg.setdefault("acceptances", []).append({
        "name": trial["trial"], "classification": classification,
        "source": source, "source_sha256": sha(PROJECT / source),
        "original_capture": str(args.original), "native_capture": str(args.native),
        "original_receipt_sha256": sha(args.original / "receipt.json"),
        "native_receipt_sha256": sha(args.native / "receipt.json"),
        "trial_sha256": sha(args.original / "trial.json"),
        "audit_sha256": sha(PROJECT / "build/audit-proof.json"),
        "code_chunks": [[row["start_rva"], row["end_rva"]] for row in trial["ranges"]
                        if row["owner"] == trial["trial"]],
        "comparison": result,
    })
    candidate["accepted"] = list(cfg["accepted"])
    candidate["trial"] = None
    candidate["stage"] = trial["trial"] + " accepted; next frontier " + result["frontier"]
    for path, value in ((PROJECT / "functions.json", cfg),
                        (PROJECT / candidate_path, candidate)):
        temporary = path.with_suffix(path.suffix + ".tmp")
        temporary.write_text(json.dumps(value, indent=2) + "\n")
        temporary.replace(path)
    print(json.dumps({"accepted": trial["trial"], "accepted_count": len(cfg["accepted"])}))


if __name__ == "__main__":
    main()
