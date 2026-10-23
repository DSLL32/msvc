#!/usr/bin/env python3
"""Audit the exact whole-function trial used by the original mask transport."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess

from inventory import ROOT, inventory

BASE = Path(__file__).resolve().parents[1]
EXPECTED_EXPORTS = [
    ("_AbortCompilerPass@4", 1), ("_InvokeCompilerPass@12", 2),
]


def verify_initializer_thunk(report, cfg, rebuilt_path=None, runtime_path=None):
    import struct
    for side, path, address in (("original", ROOT / cfg["target"]["path"], 0x104384ce),
                                ("rebuilt", rebuilt_path or BASE / "build/c1n32_l.dll", None)):
        if address is None:
            rows = json.loads((runtime_path or BASE / "build/runtime.json").read_text())
            address = next(row["start"] for row in rows if row["name"] == "__initterm")
        image = inventory(path)
        rva = address - image["image_base"]
        section = next(row for row in image["sections"] if row["rva"] <= rva < row["rva"] + row["raw_size"])
        offset = section["raw_offset"] + rva - section["rva"]
        code = path.read_bytes()[offset:offset + 6]
        if code[:2] != b"\xff\x25":
            raise ValueError("initializer boundary is not an actual six-byte indirect import thunk")
        slot = struct.unpack_from("<I", code, 2)[0]
        identity = report["import_catalogs"][side]["slots"][str(slot)]
        if identity["dll"].lower() != "msvcrt.dll" or identity.get("name") != "_initterm":
            raise ValueError("initializer thunk has wrong provider identity")
    if 'import:_ftol' in cfg.get('accepted',[]):
        for side,path,address in (('original',ROOT/cfg['target']['path'],0x10438f04),('rebuilt',rebuilt_path or BASE/'build/c1n32_l.dll',None)):
            if address is None:
                rows=json.loads((runtime_path or BASE/'build/runtime.json').read_text());address=next(row['start'] for row in rows if row['name']=='__ftol')
            image=inventory(path);rva=address-image['image_base'];section=next(row for row in image['sections'] if row['rva']<=rva<row['rva']+row['raw_size']);code=path.read_bytes()[section['raw_offset']+rva-section['rva']:section['raw_offset']+rva-section['rva']+6]
            if code[:2]!=b'\xff\x25':raise ValueError('compiler conversion boundary is not a six-byte import thunk')
            identity=report['import_catalogs'][side]['slots'][str(struct.unpack_from('<I',code,2)[0])]
            if identity['dll'].lower()!='msvcrt.dll' or identity.get('name')!='_ftol':raise ValueError('compiler conversion provider identity differs')
        if 'import:_ftol' not in report['original_accepted_graphs']:raise ValueError('compiler support absent from strict accepted-index audit')
    if "import:_initterm" not in report["original_accepted_graphs"]:
        raise ValueError("genuine import thunk absent from accepted-index audit")


def main():
    cfg = json.loads((BASE / "functions.json").read_text())
    actual = inventory(BASE / "build/c1n32_l.dll")
    exports = [(x["name"], x["ordinal"]) for x in actual["exports"]]
    if sorted(exports) != sorted(EXPECTED_EXPORTS):
        raise SystemExit("rebuilt Win32 DLL export name/ordinal catalog differs")
    if not actual["characteristics"] & 0x2000:
        raise SystemExit("rebuilt target is not a Win32 DLL")
    if actual["tls_directory"]["rva"]:
        raise SystemExit("unexpected reconstructed TLS directory")
    spec = importlib.util.spec_from_file_location("prepare_trial", Path(__file__).with_name("prepare-trial.py"))
    preparer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(preparer)
    trial = preparer.prepare()
    if trial.get("prospective_only"):
        raise SystemExit("Raw-only prospective tail chain: no acceptance graph audit or function credit")
    graph = Path(os.environ.get("GRAPH_AUDIT_BIN", ROOT / "tooling/graph-audit/target/release/graph-audit"))
    auditor_sha = hashlib.sha256(graph.read_bytes()).hexdigest()
    started = __import__("time").perf_counter()
    metadata=json.loads((BASE/"graph-metadata.json").read_text())
    register_imports=bool(metadata.get("register_imports"))
    if register_imports:
        from allocation_imports import check_static
        check_static()
    result = subprocess.run([
        str(graph), "--repo", str(ROOT), "--project", str(BASE),
        "--profile", "c1xx", "--discovery", "cfg",
        "--original", str(ROOT / cfg["target"]["path"]),
        "--rebuilt", str(BASE / "build/c1n32_l.dll"),
        "--map", str(BASE / "build/c1xx.map"),
        "--config", str(BASE / "build/trial.json"),
        "--runtime", str(BASE / "runtime.json"),
        "--rebuilt-runtime", str(BASE / "build/runtime.json"),
        "--metadata", str(BASE / "graph-metadata.json"),
        "--check-ret", "--check-imports", "--check-references", "--check-ownership", "--check-extents",
        "--include-evidenced-runtime", "--check-accepted-extents",
    ] + (["--resolve-register-imports"] if register_imports else []), check=True, stdout=subprocess.PIPE, text=True)
    elapsed = __import__("time").perf_counter() - started
    if register_imports:
        from allocation_imports import check_report
        check_report(json.loads(result.stdout))
    report = json.loads(result.stdout)
    if hashlib.sha256(graph.read_bytes()).hexdigest() != auditor_sha:
        raise SystemExit("auditor changed during execution")
    for candidate_name in trial.get("ordered_closure", [trial["trial"]] if trial["trial"] else []):
        if candidate_name.startswith("import:"):continue
        runtime = json.loads((BASE / "runtime.json").read_text())
        record = next(row for row in runtime + cfg["driver"] if row["name"] == candidate_name)
        chunks = [[record["start"], record["end"]]] + record.get("code_chunks", [])
        owned = report.get("owned_code_chunks", {}).get("original", {}).get("functions", {})
        if owned.get(candidate_name) != chunks:
            raise SystemExit("graph auditor did not prove exact candidate code-chunk ownership")
        if candidate_name not in report.get("functions", {}):
            raise SystemExit("actual candidate is absent from graph comparison")
    if "import:_initterm" in trial["accepted"]:
        verify_initializer_thunk(report, trial)
    proof = {"schema": 1, "trial": trial["trial"], "seconds": elapsed,
             "auditor_sha256": auditor_sha,
             "trial_sha256": hashlib.sha256((BASE / "build/trial.json").read_bytes()).hexdigest(),
             "report": report}
    history = BASE / "build/audits"
    history.mkdir(exist_ok=True)
    latest = BASE / "build/audit-proof.json"
    payloads = ([latest.read_bytes()] if latest.exists() else []) + [
        (json.dumps(proof, indent=2) + "\n").encode()]
    for payload in payloads:
        record = history / (hashlib.sha256(payload).hexdigest() + ".json")
        if record.exists():
            if record.read_bytes() != payload:
                raise SystemExit("immutable audit record changed")
        else:
            with record.open("xb") as stream:
                stream.write(payload)
    latest.write_bytes(payloads[-1])
    print(json.dumps({key: value for key, value in report.items()
                      if key not in ("import_catalogs", "original_accepted_graphs")}))


if __name__ == "__main__":
    main()
