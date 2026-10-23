#!/usr/bin/env python3
"""Freeze one whole-function trial for both masking and the common Rust audit."""
import hashlib
import json
from pathlib import Path
import re

from inventory import ROOT, TARGET, inventory

PROJECT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare():
    cfg = json.loads((PROJECT / "functions.json").read_text())
    candidate_path = cfg.get("candidate_path", "cases/lifecycle/candidate.json")
    candidate = json.loads((PROJECT / candidate_path).read_text())
    runtime = json.loads((PROJECT / "runtime.json").read_text())
    if candidate["accepted"] != cfg["accepted"]:
        raise ValueError("candidate accepted prefix differs from durable configuration")
    indexes = {record["name"]: record for record in cfg["driver"] + runtime + cfg.get("imports", [])}
    trial_name = candidate["trial"]
    if trial_name is not None and (trial_name not in indexes or trial_name in cfg["accepted"]):
        raise ValueError("trial is not exactly one new identified target function")
    accepted = list(cfg["accepted"]) + ([trial_name] if trial_name else [])
    base = cfg["target"]["base"]
    expected = []
    for name in accepted:
        record = indexes[name]
        for n, (first, last) in enumerate([(record["start"], record["end"])] +
                                          [tuple(pair) for pair in record.get("code_chunks", [])]):
            expected.append({"owner": name, "start_rva": first - base,
                             "end_rva": last - base, "entry": int(n == 0)})
    if candidate["ranges"] != expected:
        raise ValueError("mask ranges do not equal whole accepted/trial bodies")
    image = inventory(TARGET)
    if cfg["target"]["sha256"] != image["sha256"] or candidate["target_sha256"] != image["sha256"]:
        raise ValueError("trial target identity differs")
    ordered = sorted(expected, key=lambda item: item["start_rva"])
    for n, item in enumerate(ordered):
        if item["start_rva"] >= item["end_rva"] or not any(
                section["characteristics"] & 0x20 and section["rva"] <= item["start_rva"] and
                item["end_rva"] <= section["rva"] + min(section["virtual_size"], section["raw_size"])
                for section in image["sections"]):
            raise ValueError("trial range outside file-backed code")
        if n and ordered[n - 1]["end_rva"] > item["start_rva"]:
            raise ValueError("overlapping restored instruction ownership")
    rebuilt = inventory(PROJECT / "build/c1n32_l.dll")
    publics = {}
    pattern = re.compile(r"^\s+[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})")
    for line in (PROJECT / "build/c1xx.map").read_text().splitlines():
        match = pattern.match(line)
        if match:
            publics[match[1]] = int(match[2], 16)
    ends = sorted(set(publics.values()) | {
        rebuilt["image_base"] + section["rva"] + section["virtual_size"]
        for section in rebuilt["sections"] if section["characteristics"] & 0x20})
    rebuilt_runtime = []
    for record in runtime:
        if record["name"] not in publics:
            continue
        start = publics[record["name"]]
        rebuilt_runtime.append({"name": record["name"], "start": start,
                                "end": next(value for value in ends if value > start),
                                "evidence": record.get("evidence")})
    runtime_path = PROJECT / "build/runtime.json"
    runtime_path.write_text(json.dumps(rebuilt_runtime, indent=2) + "\n")
    trial = dict(cfg)
    trial["accepted"] = accepted
    trial["trial"] = trial_name
    trial["ranges"] = expected
    trial["case"] = candidate["case"]
    trial["candidate_sha256"] = sha(PROJECT / candidate_path)
    trial["bound_inputs"] = {item: sha(PROJECT / item) for item in (
        "functions.json", "runtime.json", "graph-metadata.json", "build/runtime.json",
        "build/c1n32_l.dll", "build/c1xx.map", "build/native/c1xx-host")}
    if trial["case"] == "minimal":
        trial["case_inputs"] = {item: sha(PROJECT / item) for item in (
            "cases/minimal/contract.json", "cases/minimal/invoke-argv.bin", "cases/sources/minimal.cpp", "cases/minimal/state-model.json")}
    trial["test_seed"] = candidate.get("test_seed")
    trial['test_constructor_seed']=bool(candidate.get('test_constructor_seed'))
    if trial["test_seed"] or trial_name in ('_checked_malloc','_register_cleanup') or (trial_name or '').startswith(('_initialize_frontend_','_initialize_output_buffer_')):
        trial["validation_observers"] = [{"owner":"_initialize_compiler_pass", "original_rva":0x385be,
            "original_byte":0x89,"purpose":"observe typed copy/restore before first-flag reset"}]
    if trial['test_constructor_seed']:
        trial['auxiliary_data_seed']={'owner':'_initialize_output_buffer_01','point':'entry before target body',
            'data':'owned snapshot+0','cursor':'owned snapshot+4','capacity':64,
            'unused_relocation':'_initialize_frontend_01; never invoked','relocation_context':'owned snapshot+8',
            'file':'owned snapshot+12; never dereferenced','ok':53,'descriptor':37,'mode':23,
            'actual_initializer_table_unchanged':True,'code_or_allocator_mock':False}
    if trial["test_seed"] == "first-copy-aux":
        trial["auxiliary_mock"] = {"callee":"_checked_malloc", "genuine_trap_required":True,
            "request":8520, "returned_buffer_role":"mock-return", "old_buffer_role":"old-snapshot",
            "owned_request":8584,"guard_bytes":32,"old_guard":0xa7,"new_guard":0xd3,
            "mocked_cleanup_callback":"_initialize_frontend_01", "mocked_cleanup_slots":32,
            "helper_unmasked":False,"helper_acceptance_credit":False}
    path = PROJECT / "build/trial.json"
    path.write_text(json.dumps(trial, indent=2) + "\n")
    return trial


if __name__ == "__main__":
    trial = prepare()
    print(json.dumps({"trial": trial["trial"], "restored_ranges": len(trial["ranges"]),
                      "accepted_durably": len(trial["accepted"]) - bool(trial["trial"])}))
