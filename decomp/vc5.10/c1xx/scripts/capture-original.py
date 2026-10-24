#!/usr/bin/env python3
"""Explicit original-only DLL capture; never imported by native fixtures."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time

from inventory import ROOT, TARGET, inventory
from importlib.util import module_from_spec, spec_from_file_location

spec = spec_from_file_location("original_build_transport", Path(__file__).with_name("wine-tool.py"))
transport = module_from_spec(spec)
spec.loader.exec_module(transport)
PROJECT = Path(__file__).resolve().parents[1]
SCRATCH = Path.home() / ".ai-tmp/msvc2/vc5.10/c1xx"
CRT = ROOT / "toolchains/5.10/bin/MSVCRT.DLL"
CRT_HASH = "e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def preflight():
    current = inventory(TARGET)
    if current != json.loads((PROJECT / "cases/pe-inventory.json").read_text()):
        raise ValueError("target changed")
    if sha(CRT) != CRT_HASH or CRT.stat().st_size != 277776:
        raise ValueError("canonical MSVCRT changed")
    if current["tls_directory"]["rva"]:
        raise ValueError("unexpected TLS prerequisite")
    data = TARGET.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    opt = pe + 24
    relocation, length = struct.unpack_from("<II", data, opt + 96 + 5 * 8)

    def raw(rva):
        for section in current["sections"]:
            delta = rva - section["rva"]
            if 0 <= delta < section["raw_size"]:
                return section["raw_offset"] + delta
        raise ValueError("unbacked relocation RVA")

    at, end = raw(relocation), raw(relocation) + length
    sites = 0
    while at < end:
        page, size = struct.unpack_from("<II", data, at)
        if size < 8 or size % 2 or at + size > end:
            raise ValueError("invalid relocation block")
        for word in struct.unpack_from("<%dH" % ((size - 8) // 2), data, at + 8):
            kind, offset = word >> 12, word & 0xfff
            if not kind:
                continue
            if kind != 3:
                raise ValueError("unsupported relocation type")
            site = page + offset
            if site <= current["entry_rva"] < site + 4:
                raise ValueError("entry guard overlaps a relocation")
            sites += 1
        at += size
    if data[raw(current["entry_rva"])] != 0x53:
        raise ValueError("entry guard changed")
    # Check locally supplied direct providers for a static cycle back to this
    # target. System loader modules are independently visible in debug events.
    local_files = {p.name.lower(): p for p in CRT.parent.iterdir() if p.is_file()}
    for item in current["imports"]:
        provider = local_files.get(item["dll"].lower())
        if provider:
            if any(x["dll"].lower() in ("c1xx.dll", "c1n32_l.dll")
                   for x in inventory(provider)["imports"]):
                raise ValueError("static provider import cycle into target")
    return {"entry_byte": 0x53, "entry_relocation_overlap": False,
            "relocation_sites": sites, "tls_directory": current["tls_directory"],
            "direct_local_import_cycle_into_target": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("baseline", "masked"))
    parser.add_argument("--name", required=True)
    parser.add_argument("--timeout", type=float, default=90)
    args = parser.parse_args()
    if not args.name.replace("-", "").replace("_", "").isalnum():
        raise ValueError("invalid fresh capture name")
    checks = preflight()
    native = PROJECT / "build/native/c1xx-host"
    native_sha = sha(native) if native.is_file() else None
    if args.mode == "masked" and native_sha is None:
        raise ValueError("masked comparison needs the current native executable")
    output = SCRATCH / args.name
    output.mkdir(parents=True, exist_ok=False)
    trial_path = PROJECT / "build/trial.json"
    if not trial_path.is_file():
        raise ValueError("prepare the exact whole-function trial before capture")
    trial = json.loads(trial_path.read_text())
    if trial.get('matcher_observers') and b'matcher-plan ordinary-candidate=1' not in (PROJECT/'build/dll-capture.exe').read_bytes():
        raise ValueError('rebuild original helper: ordinary matcher/signature observations absent')
    if trial.get('argument_observers') and b'argument-observer phase=' not in (PROJECT/'build/dll-capture.exe').read_bytes():
        raise ValueError('rebuild original helper: argument observations absent')
    if trial.get('environment_argument_observers') and b'env-arguments-observer entry=00039413' not in (PROJECT/'build/dll-capture.exe').read_bytes():
        raise ValueError('rebuild original helper: environment argument observations absent')
    if trial.get('lowercase_observers') and b'path-fold-observer entry=000393ca iat=000c9054' not in (PROJECT/'build/dll-capture.exe').read_bytes():
        raise ValueError('build/dll-capture.exe lacks required filename-fold observations; rebuild that explicit target')
    candidate_path = json.loads((PROJECT / "functions.json").read_text()).get("candidate_path", "cases/lifecycle/candidate.json")
    snapshot = [PROJECT / candidate_path, PROJECT / "functions.json",
                PROJECT / "runtime.json", PROJECT / "graph-metadata.json",
                PROJECT / "build/c1n32_l.dll", PROJECT / "build/c1xx.map",
                PROJECT / "build/runtime.json", trial_path]
    inputs = {}
    for path in snapshot:
        item = str(path.relative_to(PROJECT))
        destination = output / ("rebuilt-runtime.json" if item == "build/runtime.json" else path.name)
        shutil.copy2(path, destination)
        inputs[item] = sha(path)
    candidate = json.loads((output / "candidate.json").read_text())
    if candidate["target_sha256"] != sha(TARGET):
        raise ValueError("candidate target identity differs")
    if trial["candidate_sha256"] != sha(output / "candidate.json") or trial["ranges"] != candidate["ranges"]:
        raise ValueError("mask trial differs from frozen candidate")
    for item, digest in trial["bound_inputs"].items():
        if sha(PROJECT / item) != digest:
            raise ValueError("trial bound input changed " + item)
    (output / "ranges.txt").write_text("".join(
        "%x %x %d\n" % (row["start_rva"], row["end_rva"], row["entry"])
        for row in trial["ranges"]))
    shutil.copy2(PROJECT / "build/dll-capture.exe", output / "dll-capture.exe")
    shutil.copy2(PROJECT / "build/native/c1xx-host", output / "native-executable.elf")
    for source in ("dll-capture.c", "invoke-arguments.h", "options-fields.h", "locale-observer.inc", "path-fold-observer.inc", "vector-observer.inc", "option-action-observer.inc", "integer-observer.inc"):
        shutil.copy2(PROJECT / "scripts" / source, output / source)
    if trial.get('option_action_observers'):
        (output/'option-action-observers.txt').write_text('39632 %x\n'%(0x39757 if trial.get('integer_observers')else 0x39779))
        if b'option-actions-plan ordinary=1' not in (PROJECT/'build/dll-capture.exe').read_bytes():raise ValueError('rebuild original action observer helper')
    environment = transport.environment()
    if trial["case"] == "minimal":
        for item,digest in trial["case_inputs"].items():
            if sha(PROJECT / item) != digest: raise ValueError("case input changed " + item)
            shutil.copy2(PROJECT / item, output / Path(item).name)
        environment.update(C1XX_INVOKE_ARGV="invoke-argv.bin", TMP=".", TEMP=".")
        environment.pop("C1XX_TEST_INIT_RESTORE", None)
        environment.pop("C1XX_OBSERVE_INIT_COPY", None)
        environment.pop("C1XX_OBSERVE_ALLOCATION", None)
        if trial.get('allocation_observers'):environment['C1XX_OBSERVE_ALLOCATION']='1'
        environment.pop('C1XX_OBSERVE_EARLY_OPTIONS',None)
        if trial.get('early_switch_observers'):environment['C1XX_OBSERVE_EARLY_OPTIONS']='1'
        environment.pop('C1XX_OBSERVE_SYSTEM_INFO',None)
        if trial.get('system_info_observers'):environment['C1XX_OBSERVE_SYSTEM_INFO']='1'
        environment.pop('C1XX_OBSERVE_SCALER',None)
        if trial.get('scaler_observers'):environment['C1XX_OBSERVE_SCALER']='1'
        environment.pop('C1XX_OBSERVE_ARENA_RESET',None)
        if trial.get('arena_reset_observers'):environment['C1XX_OBSERVE_ARENA_RESET']='1'
        environment.pop('C1XX_OBSERVE_POOL_CYCLE',None)
        if trial.get('pool_cycle_observers'):environment['C1XX_OBSERVE_POOL_CYCLE']='1'
        environment.pop('C1XX_OBSERVE_OPTIONS',None)
        if trial.get('options_parser_observers'):environment['C1XX_OBSERVE_OPTIONS']='1'
        environment.pop('C1XX_OBSERVE_LANGUAGE_CONTEXT',None)
        if trial.get('language_context_observers'):environment['C1XX_OBSERVE_LANGUAGE_CONTEXT']='1'
        environment.pop('C1XX_OBSERVE_ALIAS_ROOT',None)
        if trial.get('alias_root_observers'):environment['C1XX_OBSERVE_ALIAS_ROOT']='1'
        environment.pop('C1XX_OBSERVE_CHECKSUM_TABLES',None)
        environment.pop('C1XX_OBSERVE_ISOLATED_LOCALE',None)
        if trial.get('checksum_observers'):
            environment['C1XX_OBSERVE_CHECKSUM_TABLES']='1'
            environment['C1XX_OBSERVE_ISOLATED_LOCALE']='1'
        environment.pop('C1XX_OBSERVE_MULTIBYTE',None)
        if trial.get('multibyte_observers'):environment['C1XX_OBSERVE_MULTIBYTE']='1'
        environment.pop('C1XX_OBSERVE_LOWERCASE',None)
        if trial.get('lowercase_observers'):environment['C1XX_OBSERVE_LOWERCASE']='1'
        if trial.get('environment_argument_observers'):environment['C1XX_OBSERVE_ENV_ARGUMENTS']='1'
        if trial.get('vector_observers'):environment['C1XX_OBSERVE_VECTORS']='1'
        environment.pop('C1XX_OBSERVE_ARGUMENT',None)
        if trial.get('argument_observers'):environment['C1XX_OBSERVE_ARGUMENT']='1'
        environment.pop('C1XX_OBSERVE_MATCHER',None)
        if trial.get('matcher_observers'):environment['C1XX_OBSERVE_MATCHER']='1'
        environment.pop('C1XX_TEST_ARENA_RESET',None)
        if trial.get('test_arena_reset_seed'):environment['C1XX_TEST_ARENA_RESET']='1'
        environment.pop('C1XX_TEST_OUTPUT_CONSTRUCTOR',None)
        environment.pop('C1XX_TEST_DEFERRED_CONSTRUCTOR',None)
        environment.pop('C1XX_TEST_TOKEN_COPY',None)
        environment.pop('C1XX_OBSERVE_POOL_RETURN',None)
        environment.pop('C1XX_TEST_POOL_CONSTRUCTOR',None)
        if trial['trial']=='CompilerPool::Initialize':environment['C1XX_OBSERVE_POOL_RETURN']='1'
        environment.pop('C1XX_TEST_ACTION',None)
        if trial.get('test_action_seed'):environment['C1XX_TEST_ACTION']='1'
        environment.pop('C1XX_TEST_COMMENT',None)
        if trial.get('test_comment_seed'):environment['C1XX_TEST_COMMENT']='1'
        environment.pop('C1XX_TEST_PENDING_GROUP',None)
        if trial.get('test_group_seed'):environment['C1XX_TEST_PENDING_GROUP']='1'
        environment.pop('C1XX_TEST_PCH',None)
        environment.pop('C1XX_OBSERVE_PCH_RETURN',None)
        if trial.get('test_pch_seed'):environment['C1XX_TEST_PCH']='1'
        if 'PchContext::Initialize' in trial['accepted']:environment['C1XX_OBSERVE_PCH_RETURN']='1'
        environment.pop('C1XX_TEST_PROTOTYPE',None)
        if trial.get('test_prototype_seed'):environment['C1XX_TEST_PROTOTYPE']='1'
        if trial.get('test_pool_seed'):environment['C1XX_TEST_POOL_CONSTRUCTOR']='1'
        environment.pop('C1XX_TEST_PDB_CONTEXT',None)
        environment.pop('C1XX_TEST_PDB_OBJECT',None)
        if trial.get('test_pdb_seed') is not None:
            environment['C1XX_TEST_PDB_CONTEXT']=str(trial['test_pdb_seed'])
            environment['C1XX_TEST_PDB_OBJECT']=trial['trial'][-1]
        if trial.get('test_token_seed'):environment['C1XX_TEST_TOKEN_COPY']=trial['test_token_seed']
        if trial.get('test_deferred_seed'):environment['C1XX_TEST_DEFERRED_CONSTRUCTOR']='1'
        if trial.get('test_constructor_seed'):
            environment['C1XX_TEST_OUTPUT_CONSTRUCTOR']=str(int(trial['trial'][-2:]))
        if trial.get('validation_observers'):
            environment['C1XX_OBSERVE_INIT_COPY'] = '1'
        if trial.get("test_seed"):
            environment["C1XX_TEST_INIT_RESTORE"] = trial["test_seed"]
    shutil.copy2(CRT, output / "MSVCRT.DLL")
    command = ["wine", transport.windows(output / "dll-capture.exe"), "--debug",
               transport.windows(TARGET), transport.windows(CRT), args.mode, "ranges.txt"]
    started = time.perf_counter()
    lock_wait_seconds = 0.0
    with (output / "stdout.bin").open("wb") as stdout, (output / "stderr.bin").open("wb") as stderr:
        result = subprocess.run(command, cwd=output, env=environment,
                                stdout=stdout, stderr=stderr, timeout=args.timeout)
    elapsed = time.perf_counter() - started
    for item, digest in inputs.items():
        if sha(PROJECT / item) != digest:
            raise ValueError("configuration/build changed during original capture")
    if native_sha is not None and sha(native) != native_sha:
        raise ValueError("native executable changed during original capture")
    loaded, loaded_crt = output / "loaded-target.bin", output / "loaded-msvcrt.bin"
    if loaded.exists() and sha(loaded) != sha(TARGET):
        raise ValueError("actual loaded target file differs")
    if loaded_crt.exists() and sha(loaded_crt) != CRT_HASH:
        raise ValueError("actual loaded MSVCRT file differs")
    receipt = {"schema": 1, "kind": "explicit-original-capture", "mode": args.mode,
               "command": command, "exit": result.returncode, "seconds": elapsed,
               "capture_lock_wait_seconds": lock_wait_seconds,
               "preflight": checks, "target_sha256": sha(TARGET),
               "canonical_crt_sha256": CRT_HASH, "snapshot_inputs": inputs,
               "helper_sha256": sha(output / "dll-capture.exe"),
               "artifacts": {p.name: {"sha256": sha(p), "size": p.stat().st_size}
                             for p in sorted(output.iterdir()) if p.is_file()}}
    if native_sha is not None:
        receipt["native_sha256"] = native_sha
    if trial.get("prospective_only"):
        receipt["scope"] = "prospective raw-only tail chain; no graph acceptance or function credit"
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"output": str(output), "exit": result.returncode, "seconds": elapsed}))
    return 0 if result.returncode in (0, 125) else 1


if __name__ == "__main__":
    raise SystemExit(main())
