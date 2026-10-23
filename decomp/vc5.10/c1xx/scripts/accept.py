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
    names = trial.get("ordered_closure", [trial["trial"]])
    support=trial.get('support_imports',[])
    if support:
        if trial['trial']!='_calculate_arena_reservation' or support!=['import:_ftol'] or candidate.get('support_imports')!=support:
            raise ValueError('not the reviewed sole scaler candidate with existing compiler import')
        names=[trial['trial']]+support
    if "ordered_closure" in trial:
        if names != ['_initialize_frontend_37', 'PchContext::Initialize'] or candidate.get('ordered_closure') != names or len(candidate['accepted']) != 57:
            raise ValueError('not the reviewed bounded ordered PCH closure')
    for name in names:
        source = "src/dllmain.c" if name == "_DllMain@12" else "src/startup.c"
        classification = "application source" if name == "_DllMain@12" else "target CRT support source"
        if name.startswith("import:"):
            source = next((row.get('source','src/c1xx.h')for row in cfg.get('imports',[])if row['name']==name),'src/c1xx.h')
            classification = "compiler-generated public CRT import thunk; zero application credit" if name=='import:_ftol' else "linker-generated import thunk; ordinary external declaration"
        if trial["case"] == "minimal" and not name.startswith("import:"):
            source = next(row["source"] for row in cfg["driver"] if row["name"] == name)
            classification = "application source"
        record = {
            "name": name, "classification": classification,
            "source": source, "source_sha256": sha(PROJECT / source),
            "original_capture": str(args.original), "native_capture": str(args.native),
            "original_receipt_sha256": sha(args.original / "receipt.json"),
            "native_receipt_sha256": sha(args.native / "receipt.json"),
            "trial_sha256": sha(args.original / "trial.json"),
            "audit_sha256": sha(PROJECT / "build/audit-proof.json"),
            "code_chunks": [[row["start_rva"], row["end_rva"]] for row in trial["ranges"] if row["owner"] == name and row.get('kind')!='text_data'],
            "comparison": result,
        }
        native_trial_sha=sha(args.native / 'trial.json')
        if native_trial_sha!=record['trial_sha256']:
            record['native_trial_sha256']=native_trial_sha
            record['native_only_refresh']='original raw execution reused after checked exact native ELF binding-only delta'
        data_ranges=[[row['start_rva'],row['end_rva']] for row in trial['ranges'] if row['owner']==name and row.get('kind')=='text_data']
        if name in support:
            record['application_code_credit']=0
            record['application_function_credit']=0
            record['source_projection']='automatic original compiler/linker import; ordinary native signed64 cast'
        if data_ranges:
            record['shared_text_data']=data_ranges
            record['shared_text_data_code_credit']=0
        if 'ordered_closure' in trial:
            record['promotion_kind'] = 'reviewed ordered JOINT two-body closure; not a historical single-function proof'
            record['ordered_closure'] = names
        cfg.setdefault("acceptances", []).append(record)
    candidate["accepted"] = list(cfg["accepted"])
    candidate["trial"] = None
    candidate.pop("ordered_closure", None)
    candidate.pop("support_imports",None)
    candidate.pop("prospective_only", None)
    candidate["stage"] = trial["trial"] + " accepted; next frontier " + result["frontier"]
    for path, value in ((PROJECT / "functions.json", cfg),
                        (PROJECT / candidate_path, candidate)):
        temporary = path.with_suffix(path.suffix + ".tmp")
        temporary.write_text(json.dumps(value, indent=2) + "\n")
        temporary.replace(path)
    print(json.dumps({"accepted": trial["trial"], "accepted_count": len(cfg["accepted"]), "promoted_functions": names}))


if __name__ == "__main__":
    main()
