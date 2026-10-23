"""Shared immutable raw-evidence readers for the lifecycle trial decoders."""
import hashlib
import json
from pathlib import Path
import re
import struct

PROJECT = Path(__file__).resolve().parents[1]
ROOT = PROJECT.parents[2]
# Set only by the explicit read-only historical validator; acceptance leaves this None.
HISTORICAL_SNAPSHOT = None

INPUTS = ("cases/lifecycle/candidate.json", "functions.json", "runtime.json",
          "graph-metadata.json", "build/c1n32_l.dll", "build/c1xx.map",
          "build/runtime.json", "build/trial.json")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def frozen_name(item):
    return "rebuilt-runtime.json" if item == "build/runtime.json" else Path(item).name


def receipt(directory, original=False):
    value = json.loads((directory / "receipt.json").read_text())
    frozen_cfg = json.loads((directory / "functions.json").read_text())
    inputs = tuple(frozen_cfg.get("candidate_path", "cases/lifecycle/candidate.json") if item == "cases/lifecycle/candidate.json" else item for item in INPUTS)
    require(set(value["snapshot_inputs"]) == set(inputs), "incomplete frozen trial inputs")
    for item in inputs:
        path = directory / frozen_name(item)
        require(sha(path) == value["snapshot_inputs"][item], "historical input changed " + item)
        if HISTORICAL_SNAPSHOT is None and (item != "build/trial.json" or not original):
            require(path.read_bytes() == (PROJECT / item).read_bytes(), "execution input changed " + item)
    for name, record in value["artifacts"].items():
        require(Path(name).name == name, "artifact escaped capture directory")
        path = directory / name
        require(path.is_file() and sha(path) == record["sha256"] and path.stat().st_size == record["size"],
                "raw artifact changed " + name)
    if not original:
        if HISTORICAL_SNAPSHOT is None:
            require(value["native_sha256"] == sha(PROJECT / "build/native/c1xx-host"), "stale native executable")
        provenance = directory / "build-provenance.json"
        require(sha(provenance) == value["compiled_source_provenance_sha256"], "compiled-source record changed")
        build = json.loads(provenance.read_text())
        require(build["artifact_sha256"] == value["native_sha256"], "provenance belongs to another native executable")
        pending, seen = list(build["source_content"].values()), set()
        while pending:
            digest = pending.pop()
            if digest in seen:
                continue
            seen.add(digest)
            require(re.fullmatch('[0-9a-f]{64}',digest), "invalid source content identity")
            content = directory / "source-content" / digest
            require(sha(content) == digest, "compiled source content changed")
            try:
                nested = json.loads(content.read_bytes())
            except (UnicodeDecodeError, ValueError):
                continue
            if isinstance(nested, dict) and "source_content" in nested:
                pending.extend(nested["source_content"].values())
    return value


def pair(original, native):
    o, n = receipt(original, True), receipt(native)
    require(o["kind"] == "explicit-original-capture" and o["mode"] == "masked",
            "not masked original evidence")
    trial = json.loads((native / "trial.json").read_text())
    old_trial = json.loads((original / "trial.json").read_text())
    require(old_trial["bound_inputs"]["build/native/c1xx-host"] == o["native_sha256"],
            "historical native binding is inconsistent")
    old_core = {key: value for key, value in old_trial.items() if key != "bound_inputs"}
    current_core = {key: value for key, value in trial.items() if key != "bound_inputs"}
    require(old_core == current_core, "original target/mask/candidate/execution semantics changed")
    require(trial["bound_inputs"]["build/native/c1xx-host"] == n["native_sha256"], "native execution binding differs")
    for item, digest in trial["bound_inputs"].items():
        if HISTORICAL_SNAPSHOT is not None and item == "build/native/c1xx-host":
            continue
        path = HISTORICAL_SNAPSHOT / frozen_name(item) if HISTORICAL_SNAPSHOT else PROJECT / item
        require(sha(path) == digest, "candidate build changed " + item)
    for item,digest in trial.get("case_inputs", {}).items():
        require(sha(native / Path(item).name) == sha(original / Path(item).name) == digest,
                "frozen case input differs " + item)
        if HISTORICAL_SNAPSHOT is None:
            require(sha(PROJECT / item) == digest, "current case input changed " + item)
    target = ROOT / "toolchains/5.10/bin/C1XX.DLL"
    crt = ROOT / "toolchains/5.10/bin/MSVCRT.DLL"
    require(sha(original / "loaded-target.bin") == o["target_sha256"] == sha(target) == trial["target"]["sha256"],
            "actual target module differs")
    require(sha(original / "loaded-msvcrt.bin") == o["canonical_crt_sha256"] == sha(crt) ==
            "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210", "actual CRT provider differs")
    expected_helper = sha(original / "dll-capture.exe") if HISTORICAL_SNAPSHOT else sha(PROJECT / "build/dll-capture.exe")
    require(sha(original / "dll-capture.exe") == o["helper_sha256"] == expected_helper,
            "original controller changed")
    text = (original / "events.txt").read_text()
    require("failure " not in text, "original controller failure")
    modules = re.findall(r"^module name=c1n32_l.dll actual=([0-9a-f]{8}) preferred=([0-9a-f]{8}) entry=([0-9a-f]{8})$", text, re.M)
    require(len(modules) == 1 and modules[0] == ("10400000", "10400000", "0002e3c6"),
            "unknown module base; nonpreferred certification is pending")
    snapshot = (original / "relocated-text.bin").read_bytes()
    require(len(snapshot) == 0xb7670 and snapshot[0x2e3c6 - 0x1000] == 0x53, "bad relocated snapshot")
    masked = bytearray(b"\xcc" * len(snapshot))
    for row in trial["ranges"]:
        a, b = row["start_rva"] - 0x1000, row["end_rva"] - 0x1000
        require(0 <= a < b <= len(snapshot), "mask range outside code")
        masked[a:b] = snapshot[a:b]
        if row["entry"]:
            masked[a] = 0xcc
    require((original / "masked-text.bin").read_bytes() == masked, "actual mask/full code restoration differs")
    state = json.loads((native / "state.json").read_text())
    require(state["registration"] == {"name": "C1XX.DLL", "path": "R:\\toolchains\\5.10\\bin\\C1XX.DLL",
                                     "sha256": sha(target), "flags": 2}, "wrong registered module provenance")
    require(state["registered_token"] != 0, "missing registered module")
    for stream in ("stdout.bin", "stderr.bin"):
        require((original / stream).read_bytes() == (native / stream).read_bytes(), "raw partial output differs")
    return o, n, trial, text, state


def original_state(directory, text, sequence):
    raw = (directory / ("state-%03d.bin" % sequence)).read_bytes()
    require(len(raw) == 0xa4, "missing raw target data")
    word = lambda offset: struct.unpack_from("<I", raw, offset)[0]
    heap = re.findall(r"^state seq=%d heap-begin=([0-9a-f]{8}) heap-first=([0-9a-f]{8})$" % sequence, text, re.M)
    require(len(heap) == 1 and int(heap[0][0], 16) == word(0x1c), "heap record differs from target pointer")
    return {"process_attached": struct.unpack_from("<i", raw, 0xc)[0],
            "initializer_words": [word(0x10), word(0x14)], "dll_adjust_fdiv": word(0x18),
            "onexit_begin": word(0x1c), "onexit_end": word(0x20),
            "raw_dll_entry": word(0x24), "dll_module": word(0x28),
            "onexit_first": int(heap[0][1], 16) if word(0x1c) else None}


def audit(trial, audit_path=None):
    proof = json.loads((audit_path or PROJECT / "build/audit-proof.json").read_text())
    trial_path = HISTORICAL_SNAPSHOT / "trial.json" if HISTORICAL_SNAPSHOT else PROJECT / "build/trial.json"
    require(proof["trial"] == trial["trial"] and proof["trial_sha256"] == sha(trial_path),
            "audit did not compare the current complete trial")
    require(proof["auditor_sha256"] == sha(ROOT / "tooling/graph-audit/target/release/graph-audit"),
            "auditor changed; re-audit only")
    report = proof["report"]
    inputs = HISTORICAL_SNAPSHOT or PROJECT
    runtime = json.loads((inputs / "runtime.json").read_text())
    cfg = json.loads((inputs / "functions.json").read_text())
    drivers = cfg["driver"]
    owners = {row["name"]: [[row["start"], row["end"]]] + row.get("code_chunks", [])
              for row in runtime + drivers if row["name"] in trial["accepted"]}
    source_count = len(owners)
    owners.update({row["name"]: [[row["start"],row["end"]]] for row in cfg.get("imports",[])
                   if row["name"] in trial["accepted"]})
    require(report["checked_functions"] == source_count and
            report["owned_code_chunks"]["original"]["functions"] == owners, "incomplete logical graph/chunk audit")
    return proof
