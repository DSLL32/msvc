#!/usr/bin/env python3
"""Check first CRT-entry trial's raw next frontier and typed state; no promotion."""
import argparse
import importlib.util
import json
from pathlib import Path
from provenance import captured_native_binary
from audit_roles import expected_code, text_bytes, validate_config
import re
import struct
import subprocess

spec = importlib.util.spec_from_file_location("pair", Path(__file__).with_name("check-pair.py"))
pair = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pair)
PROJECT = pair.PROJECT


def check(original, native, audit_directory=None, prospective_tail_chain=False):
    o, n = pair.checked_receipt(original), pair.checked_receipt(native)
    if prospective_tail_chain:
        assert o['kind']=='original-prospective-tail-chain' and o['mode']=='masked' and o['exit']==125
        assert n['kind']=='native-prospective-tail-chain'
    else:
        assert o["kind"] == "explicit-original-capture" and o["mode"] == "masked" and o["exit"] in (0, 125)
        assert n["kind"] in ("native-gdb-frontier", "native-gdb-completion")
    assert o.get("probe", "normal") == n.get("probe", "normal")
    assert o.get("value_copy_sign") == n.get("value_copy_sign")
    assert o.get("pdb_reuse") == n.get("pdb_reuse")
    candidate = json.loads((original / "candidate.json").read_text())
    config = json.loads((original / "functions.json").read_text())
    assert candidate["functions"] and config["driver"], "trial itself is absent"
    if prospective_tail_chain:
        from prospective import check as check_prospective
        assert check_prospective(original,candidate)==check_prospective(native,candidate)
        assert o['helper_sha256']==pair.sha(PROJECT/'build/dll-capture.exe')
    else:
        default_tool_path = "tooling/graph-audit/target/release/graph-audit"
        native_path = "decomp/vc5.10/c1/build/native/c1-host"
        captured_inputs = []
        for directory, receipt in ((original, o), (native, n)):
            assert receipt["audit_binding_sha256"] == pair.sha(directory / "audit-binding.json")
            captured_binding = json.loads((directory / "audit-binding.json").read_text())
            captured_tool_path = captured_binding.get("auditor_path", default_tool_path)
            ignored = {captured_tool_path} | ({native_path} if receipt["kind"] == "explicit-original-capture" else set())
            execution_inputs = {path: value for path, value in captured_binding["inputs"].items() if path not in ignored}
            execution_inputs = validate_config(directory,candidate,execution_inputs)
            assert all(pair.sha(pair.ROOT / path) == value for path, value in execution_inputs.items())
            assert pair.sha(directory / "audit.json") == captured_binding["report_sha256"]
            captured_inputs.append(execution_inputs)
        assert captured_inputs[0] == {path: value for path, value in captured_inputs[1].items() if path != native_path}
        execution_inputs = captured_inputs[1]
        assert o["helper_sha256"] == pair.sha(PROJECT / "build/dll-capture.exe")
        audit_directory = audit_directory or PROJECT / "build"
        binding = json.loads((audit_directory / "trial-audit-binding.json").read_text())
        tool_path = binding.get("auditor_path", default_tool_path)
        assert all(pair.sha(pair.ROOT / path) == value for path, value in binding["inputs"].items())
        assert {path: value for path, value in binding["inputs"].items() if path != tool_path} == execution_inputs
        assert pair.sha(audit_directory / "trial-audit.json") == binding["report_sha256"]
        report = json.loads((audit_directory / "trial-audit.json").read_text())
        assert {f["name"] for f in candidate["functions"]} == {f["name"] for f in config["driver"]}
        chunks = sorted(tuple(chunk) for f in candidate["functions"] for chunk in f["chunks"])
        chunks += [(item["start_rva"], item["end_rva"]) for item in candidate.get("imports", [])]
        chunks.sort()
        assert chunks == sorted((r["start_rva"], r["end_rva"]) for r in candidate["ranges"])
        assert all(start < end for start, end in chunks)
        assert all(left[1] <= right[0] for left, right in zip(chunks, chunks[1:]))
        expected_owned = expected_code(candidate)
        owned = report["owned_code_chunks"]["original"]
        assert owned["functions"] == expected_owned
        assert owned["logical_function_count"] == len(expected_owned)
        assert report["checked_functions"] == len(candidate["functions"])
        assert owned["interval_count"] == sum(len(v) for v in expected_owned.values())
        assert owned["declared_code_bytes"] == sum(b - a for v in expected_owned.values() for a,b in v)
        assert {"call_graph", "known_imports", "ret_cleanup", "extent_ownership", "whole_extent_boundaries"} <= set(report["checks"])
    assert pair.sha(original / "loaded-target.bin") == o["target_sha256"] == config["target"]["sha256"] == pair.sha(pair.ROOT / config["target"]["path"])
    assert pair.sha(original / "loaded-msvcrt.bin") == o["canonical_crt_sha256"] == pair.sha(pair.ROOT / "toolchains/5.10/bin/MSVCRT.DLL")
    if candidate.get('audit_metadata'):
        from allocation_imports import check_static,check_report
        check_static();check_report(report)
    expected = candidate["expected_frontier"]
    if expected.get("kind") == "completion":
        from completion_state import validate
        result = validate(original, native, o, n)
        result.update({"rust_checked_functions": report["checked_functions"],
                       "rust_owned_original_bytes": owned["declared_code_bytes"] + text_bytes(candidate),
            "rust_owned_code_bytes": owned["declared_code_bytes"],
            "rust_owned_text_data_bytes": text_bytes(candidate),
                       "current_auditor_sha256": binding["inputs"][tool_path],
                       "current_audit_report_sha256": binding["report_sha256"],
                       "execution_receipts": {"original": pair.sha(original / "receipt.json"),
                                              "native": pair.sha(native / "receipt.json")}})
        return result
    assert o["exit"] == 125 and n["kind"] == ("native-prospective-tail-chain" if prospective_tail_chain else "native-gdb-frontier")
    if expected.get('kind')=='option-vectors-complete':
        assert not prospective_tail_chain
        from vector_state import validate as validate_vectors
        result=validate_vectors(original,native,candidate,report)
        result.update(rust_checked_functions=report['checked_functions'],rust_owned_code_bytes=owned['declared_code_bytes'],rust_owned_text_data_bytes=text_bytes(candidate),rust_owned_original_bytes=owned['declared_code_bytes']+text_bytes(candidate),current_auditor_sha256=binding['inputs'][tool_path],current_audit_report_sha256=binding['report_sha256'])
        return result
    if expected.get('kind')=='environment-arguments-complete':
        assert not prospective_tail_chain
        from environment_argument_state import validate as validate_environment
        result=validate_environment(original,native,candidate,report)
        result.update(rust_checked_functions=report['checked_functions'],rust_owned_code_bytes=owned['declared_code_bytes'],rust_owned_text_data_bytes=text_bytes(candidate),rust_owned_original_bytes=owned['declared_code_bytes']+text_bytes(candidate),current_auditor_sha256=binding['inputs'][tool_path],current_audit_report_sha256=binding['report_sha256'])
        return result
    if expected.get('kind')=='path-fold-complete':
        assert not prospective_tail_chain
        from path_fold_state import validate as validate_path_fold
        result=validate_path_fold(original,native,candidate,report)
        result.update(rust_checked_functions=report['checked_functions'],rust_owned_code_bytes=owned['declared_code_bytes'],rust_owned_text_data_bytes=text_bytes(candidate),rust_owned_original_bytes=owned['declared_code_bytes']+text_bytes(candidate),current_auditor_sha256=binding['inputs'][tool_path],current_audit_report_sha256=binding['report_sha256'])
        return result
    if expected.get('kind')=='multibyte-scanner-complete':
        assert not prospective_tail_chain
        from multibyte_state import validate as validate_multibyte
        result=validate_multibyte(original,native,candidate,report)
        result.update(rust_checked_functions=report['checked_functions'],rust_owned_code_bytes=owned['declared_code_bytes'],rust_owned_text_data_bytes=text_bytes(candidate),rust_owned_original_bytes=owned['declared_code_bytes']+text_bytes(candidate),current_auditor_sha256=binding['inputs'][tool_path],current_audit_report_sha256=binding['report_sha256'])
        return result
    if expected.get('kind')=='public-crt-entry':
        assert not prospective_tail_chain
        from checksum_state import validate as validate_checksum
        result=validate_checksum(original,native,candidate,report)
        result.update(rust_checked_functions=report['checked_functions'],rust_owned_code_bytes=owned['declared_code_bytes'],rust_owned_text_data_bytes=text_bytes(candidate),rust_owned_original_bytes=owned['declared_code_bytes']+text_bytes(candidate),current_auditor_sha256=binding['inputs'][tool_path],current_audit_report_sha256=binding['report_sha256'])
        return result
    raw = (original / "events.txt").read_text()
    frontiers = re.findall(r"^frontier rva=([0-9a-f]{8})$", raw, re.M)
    traps = re.findall(r"^trap seq=(\d+) rva=([0-9a-f]{8}).*stack=([0-9a-f,]+)\nlast-error=(\d+)$", raw, re.M)
    assert len(frontiers) == 1 and traps, "missing raw next frontier"
    sequence, rva, words, error = traps[-1]
    assert int(frontiers[0], 16) == int(rva, 16) == expected["rva"]
    assert "failure " not in raw
    assert int(traps[0][1], 16) == candidate["functions"][0]["chunks"][0][0]
    state = json.loads((native / "state.json").read_text())
    assert state["symbol"] == expected["native_symbol"]
    assert len(state["formal_inventory"]) == len(state["args"])
    assert {row["name"] for row in state["formal_inventory"]} == set(state["args"])
    assert all(row["type"] for row in state["formal_inventory"])
    transcript = (native / "gdb.txt").read_text()
    import_boundary = expected.get("kind") == "import-boundary"
    if import_boundary:
        assert "Breakpoint 1, " + state["symbol"] + " (" in transcript
        assert "break recon_initterm" in (native / "commands.gdb").read_text()
    else:
        assert "Program received signal SIGTRAP" in transcript
        assert re.search(r"^(?:0x[0-9a-f]+ in )?" + re.escape(state["symbol"]) + r" \(", transcript, re.M)
    symbols = subprocess.check_output(["nm", "-S", "--defined-only", str(PROJECT / "build/native/c1-host")], text=True)
    match = re.search(r"^([0-9a-f]+) ([0-9a-f]+) [TtWw] " + re.escape(expected.get("native_linkage_symbol", state["symbol"])) + "$", symbols, re.M)
    assert match
    begin, size = (int(value, 16) for value in match.groups())
    assert begin < state["pc"] <= begin + size
    if not import_boundary:
        opcode = subprocess.check_output(["objdump", "-d", "--start-address=" + str(state["pc"] - 1),
                                          "--stop-address=" + str(state["pc"]), str(PROJECT / "build/native/c1-host")], text=True)
        assert "int3" in opcode
    args = [int(value, 16) for value in words.split(",")][1:4]
    module = re.search(r"^module name=c1n32_l.dll actual=([0-9a-f]+) preferred=([0-9a-f]+)", raw, re.M)
    assert module
    module_base = int(module[1], 16)
    identity = state["module_identity"]
    assert identity == {"name": "C1.DLL", "path": "R:\\toolchains\\5.10\\bin\\C1.DLL",
                         "sha256": o["target_sha256"], "flags": 2}
    assert state["module_token"] != 0
    pass_boundary = expected.get("kind") == "compiler-pass-entry"
    pass_initializer = expected.get("kind") == "pass-initializer"
    compiler_allocation = expected.get("kind") == "compiler-allocation"
    initialization_callback = expected.get("kind") in ("initialization-callback", "initialization-helper")
    compiler_main = expected.get("kind") == "compiler-main"
    main_helper = expected.get("kind") == "main-helper"
    work_allocation = expected.get('kind') == 'work-buffer-allocation'
    main_switches = expected.get('kind') == 'main-switches'
    scanner_argument=expected.get('kind')=='scanner-argument'
    post_internal=expected.get('kind')=='post-internal-switches'
    post_system=expected.get('kind')=='post-system-info'
    arena_initialization=expected.get('kind') in ('arena-initialization','arena-reservation','arena-size-calculation','arena-initialized','arenas-complete','default-pool-arena','pool-refill','arena-page-allocation','arena-commit','command-line-entry','command-line-helper')
    compiler_flow = arena_initialization or post_system or post_internal or scanner_argument or main_switches or work_allocation or pass_boundary or pass_initializer or compiler_allocation or initialization_callback or compiler_main or main_helper
    if compiler_flow:
        case = candidate.get("case_contract") or json.loads((PROJECT / "cases/minimal/case.json").read_text())
        pass_state = state if pass_boundary else state["caller"] if pass_initializer else state["invocation"]
        if compiler_main:
            assert state['caller']['symbol']=='InvokeCompilerPass'
            assert set(state['args'])=={'argc','argv'}
            assert state['args']=={key:pass_state['args'][key] for key in ('argc','argv')}
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            invoke_words=[int(value,16) for value in invocations[0][2].split(',')]
            frontier_words=[int(value,16) for value in words.split(',')]
            assert frontier_words[1:3]==invoke_words[1:3] and frontier_words[0]==module_base+0x12274
            snapshot=(original/'relocated-text.bin').read_bytes()
            assert snapshot[0x1226f-0x1000]==0xe8 and 0x12274+struct.unpack_from('<i',snapshot,0x12270-0x1000)[0]==expected['rva']==0x127a7
            assert state['argv']==pass_state['argv'] and state['argv_terminator']==0
            assert state['pass_mode']==case['BOOL']
            args=invoke_words[1:4]
        if main_helper:
            assert state['caller']['symbol']=='c1_main' and state['args']=={}
            assert state['caller']['args']=={key:pass_state['args'][key] for key in ('argc','argv')}
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            invoke_words=[int(value,16) for value in invocations[0][2].split(',')]
            assert int(words.split(',')[0],16)==module_base+0x127b8 and expected['rva']==0x12830
            snapshot=(original/'relocated-text.bin').read_bytes()
            assert snapshot[0x127b3-0x1000]==0xe8 and 0x127b8+struct.unpack_from('<i',snapshot,0x127b4-0x1000)[0]==expected['rva']
            assert state['pass_mode']==case['BOOL']
            args=invoke_words[1:4]
        if work_allocation:
            assert state['caller']['symbol']=='initialize_work_buffers'
            assert state['args']=={'reserve_bytes':0x500000,'commit_bytes':0x4000}
            assert expected['native_args']==['reserve_bytes','commit_bytes']
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            args=[int(value,16) for value in invocations[0][2].split(',')][1:4]
        if main_switches:
            assert state['caller']['symbol']=='c1_main'
            assert state['args']=={'arguments':pass_state['args']['argv']}
            assert state['caller']['args']=={key:pass_state['args'][key] for key in ('argc','argv')}
            assert int(words.split(',')[0],16)==module_base+0x127df
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            args=[int(value,16) for value in invocations[0][2].split(',')][1:4]
            assert [int(value,16) for value in words.split(',')][1]==args[1]
        if scanner_argument:
            assert state['caller']['symbol']=='scan_internal_switches'
            assert set(state['args'])=={'argument','mode'} and state['args']['mode']==0
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            args=[int(value,16)for value in invocations[0][2].split(',')][1:4]
        if post_internal:
            assert state['caller']['symbol']=='c1_main' and state['args']=={}
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            args=[int(value,16)for value in invocations[0][2].split(',')][1:4]
        if post_system:
            assert state['caller']['symbol']=='c1_main' and state['args']=={}
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            args=[int(value,16)for value in invocations[0][2].split(',')][1:4]
        if arena_initialization:
            assert state['caller']['symbol']==expected['caller']['native_symbol']
            if expected['kind'] in ('arena-initialization','arenas-complete','command-line-helper'):assert state['args']=={}
            elif expected['kind']=='arena-reservation':assert set(state['args'])=={'this','descriptor','address'}
            elif expected['kind']=='arena-initialized':assert set(state['args'])=={'this'}
            elif expected['kind']=='default-pool-arena':assert set(state['args'])=={'this','arena_index'}
            elif expected['kind']=='pool-refill':assert set(state['args'])=={'this','minimum_bytes'}
            elif expected['kind']=='arena-page-allocation':assert set(state['args'])=={'this','pages','available_bytes'}
            elif expected['kind']=='arena-commit':assert set(state['args'])=={'this'}
            elif expected['kind']=='command-line-entry':assert state['args']=={key:pass_state['args'][key]for key in ('argc','argv')}
            else:assert set(state['args'])=={'descriptor'}
            invocations=[row for row in traps if int(row[1],16)==0x12229];assert len(invocations)==1
            args=[int(value,16)for value in invocations[0][2].split(',')][1:4]
        if pass_initializer:
            assert state["args"] == {} and pass_state["symbol"] == "InvokeCompilerPass"
            invocations = [row for row in traps if int(row[1], 16) == 0x12229]
            assert len(invocations) == 1
            args = [int(value, 16) for value in invocations[0][2].split(",")][1:4]
            assert state["pass_mode"] == struct.unpack_from("<i", (original / ("state-%03d.bin" % int(sequence))).read_bytes(), 0x8c)[0] == 0
        if compiler_allocation or initialization_callback:
            if compiler_allocation:
                assert args[0] == state["args"]["size"] == 6548
                assert state["caller"]["symbol"] == "initialize_compiler_pass"
            elif expected.get("kind") == "initialization-helper":
                caller = expected.get("caller", {"native_symbol": "initialize_state_123a7",
                         "return_rva": 0x123f2 if expected["rva"] == 0x123f6 else 0x123ac})
                assert state["caller"]["symbol"] == caller["native_symbol"]
                owner = next(row for row in candidate["functions"] if row["native_symbol"] == caller["native_symbol"])
                snapshot = (original / "relocated-text.bin").read_bytes()
                if caller.get("tailcall"):
                    assert expected["calling_convention"] == "thiscall-tail" and expected["rva"] == 0x11e2a
                    assert owner["chunks"] == [[0x1279c, 0x127a6]] and caller["tail_rva"] == 0x127a1
                    assert caller["inherited_return_rva"] == 0x12348
                    assert int(words.split(",")[0], 16) == module_base + 0x12348
                    assert snapshot[0x1279c-0x1000:0x127a1-0x1000] == b"\xb9" + struct.pack("<I", module_base+0x58f68)
                    assert snapshot[0x127a1-0x1000] == 0xe9
                    displacement = struct.unpack_from("<i", snapshot, 0x127a2-0x1000)[0]
                    assert 0x127a6 + displacement == expected["rva"]
                else:
                    assert any(a <= caller["return_rva"] - 1 < b for a, b in owner["chunks"])
                    assert int(words.split(",")[0], 16) == module_base + caller["return_rva"]
                    call_site = caller["return_rva"] - 5
                    assert snapshot[call_site - 0x1000] == 0xe8
                    displacement = struct.unpack_from("<i", snapshot, call_site - 0x1000 + 1)[0]
                    assert caller["return_rva"] + displacement == expected["rva"]
                if expected["rva"] == 0x123f6:
                    cb = expected.get("callback", {"native_symbol": "destroy_output_buffer_23940", "original_rva": 0x23940})
                    callback = re.search(r"^([0-9a-f]+) [0-9a-f]+ [Tt] " + re.escape(cb["native_symbol"]) + "$", symbols, re.M)
                    assert callback and state["args"] == {"callback": int(callback[1], 16)}
                    assert args[0] == module_base + cb["original_rva"]
                elif expected.get("calling_convention") == "thiscall-tail":
                    assert set(state["args"]) == {"this"}
                elif expected.get("calling_convention") == "thiscall":
                    assert state["args"]["minimum_pages"] == args[0] == 1
                else:
                    assert state["args"] == {}
            else:
                assert state["args"] == {} and state["caller"]["symbol"] == "initialize_compiler_pass"
            assert pass_state["symbol"] == "InvokeCompilerPass"
            invocations = [row for row in traps if int(row[1], 16) == 0x12229]
            assert len(invocations) == 1
            args = [int(value, 16) for value in invocations[0][2].split(",")][1:4]
        assert args[0] == pass_state["args"]["argc"] == case["argc"]
        assert args[2] == pass_state["args"]["mode"] == case["BOOL"]
        assert pass_state["argv_terminator"] == 0
        actual = [(original / ("invoke-arg-%03d.bin" % i)).read_bytes() for i in range(args[0])]
        assert all(item.endswith(b"\0") and b"\0" not in item[:-1] for item in actual)
        assert [item[:-1].decode("ascii") for item in actual] == pass_state["argv"] == case["argv"]
    elif import_boundary:
        assert args[:2] == [module_base + 0x5c000, module_base + 0x5c004]
        assert state["args"] == {"begin": state["initializer_begin"], "end": state["initializer_end"]}
        assert state["initializer_end"] - state["initializer_begin"] == 4
        assert state["initializer_slots"] == [0]
        assert state["caller"]["symbol"] == "c1_crt_init"
        caller = state["caller"]["args"]
        assert caller == {"module": state["module_token"], "reason": 1, "reserved": 0}
        prior = [row for row in traps if int(row[1], 16) == 0x120ce]
        assert len(prior) == 1
        assert [int(value, 16) for value in prior[0][2].split(",")][1:4] == [module_base, 1, 0]
    else:
        assert args[0] == module_base
        assert state["args"]["module"] == state["module_token"]
        assert [state["args"][name] for name in ("reason", "reserved")] == args[1:3]
    globals_data = (original / ("state-%03d.bin" % int(sequence))).read_bytes()
    attached = struct.unpack_from("<i", globals_data, 0x90)[0]
    hook = struct.unpack_from("<I", globals_data, 0x9c)[0]
    expected_attached = 1 if expected["native_symbol"] == "DllMain" or compiler_flow else 0
    assert attached == state["crt_attached"] == expected_attached
    assert hook == state["raw_dllmain"] == 0, "nonnull callbacks need an explicit identity projection"
    assert int(error) == state["win32_last_error"] == (0 if compiler_flow else 14007)
    control = re.search(r"^trap seq=" + sequence + r" .*\nlast-error=\d+\nx87-control=([0-9a-f]+)$", raw, re.M)
    # NT stores this architectural WORD in a DWORD context field.
    assert control
    assert (int(control[1], 16) & 0xffff) == state["x87_control"] == 0x027f
    if import_boundary or expected["native_symbol"] == "DllMain":
        end, begin = struct.unpack_from("<2I", globals_data, 0x94)
        assert begin == end != 0
        heap = re.search(r"^state seq=" + sequence + r" heap-begin=([0-9a-f]+) heap-first=([0-9a-f]+)$", raw, re.M)
        assert heap and int(heap[1], 16) == begin and int(heap[2], 16) == 0
        assert state["onexit_begin"] == state["onexit_end"] != 0 and state["onexit_first"] == 0
        assert struct.unpack_from("<I", globals_data, 0)[0] == 0
        assert struct.unpack_from("<I", globals_data, 0x40)[0] == state["dll_instance"] == 0
        assert struct.unpack_from("<i", globals_data, 0xa0)[0] == state["crt_adjust_fdiv"] == 0
        assert "provider iat=0005d0e8 resolved=78001715" in raw
    if compiler_flow:
        end, begin = struct.unpack_from("<2I", globals_data, 0x94)
        assert begin == end != 0 and state["onexit_begin"] == state["onexit_end"] != 0
        assert state["onexit_first"] == 0
        assert struct.unpack_from("<I", globals_data, 0x40)[0] == module_base
        assert state["dll_instance"] == state["module_token"]
    if candidate.get("field_map"):
        from compiler_state import validate
        validate(original, native, state, int(sequence), module_base, candidate, expected, o.get("probe", "normal"))
    for item in candidate.get("imports", []):
        snapshot = (original / "relocated-text.bin").read_bytes()
        offset = item["start_rva"] - 0x1000
        assert snapshot[offset:offset + 6] == b"\xff\x25" + struct.pack("<I", module_base + item["iat_rva"])
        assert item["end_rva"] - item["start_rva"] == 6
        if not prospective_tail_chain:
            slot = report["import_catalogs"]["original"]["slots"][str(0x10600000 + item["iat_rva"])]
            assert slot == {"dll": item["provider"], "name": item.get("import_name", "_initterm")}
    labels = ["module identity/token", "CRT attach count", "null raw callback", "Win32 last error", "x87 control WORD"]
    labels.append("actual pass argc/argv/BOOL and NUL terminator" if compiler_flow else "reason/reserved")
    if candidate.get("field_map"):
        labels += ["complete34-slot callback identity table", "modeled field map/storage nonoverlap",
                   "typed initialized/BSS/snapshot state; other bytes UNMODELED"]
    if prospective_tail_chain:
        return {'status':'matching prospective wrapper-to-member input; NOT acceptance',
                'wrapper_rva':0x1279c,'frontier_rva':expected['rva'],'state':labels,
                'promotion':False,'audit':'unresolved pending tail member; strict joint audit still required',
                'execution_receipts':{'original':pair.sha(original/'receipt.json'),'native':pair.sha(native/'receipt.json')}}
    return {"status": "fresh matching next frontier and typed CRT state",
            "trial": [f["name"] for f in candidate["functions"]],
            "frontier_rva": expected["rva"], "native_symbol": state["symbol"],
            "state": labels,
            "captured_native_elf": ({"path": str(captured_native_binary(native)), "sha256": n["native_sha256"]} if candidate.get("field_map") else None),
            "rust_checked_functions": report["checked_functions"],
            "rust_owned_original_bytes": owned["declared_code_bytes"] + text_bytes(candidate),
            "rust_owned_code_bytes": owned["declared_code_bytes"],
            "rust_owned_text_data_bytes": text_bytes(candidate),
            "current_auditor_sha256": binding["inputs"][tool_path],
            "current_audit_report_sha256": binding["report_sha256"],
            "execution_receipts": {"original": pair.sha(original / "receipt.json"),
                                   "native": pair.sha(native / "receipt.json")},
            "promotion": ("held for orchestrator/root initializer review" if any("held" in f.get("status", "") for f in candidate["functions"]) else "eligible under reviewed same-protocol continuation")}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("native", type=Path)
    args = parser.parse_args()
    result = check(args.original, args.native)
    (PROJECT / "build/trial-validation.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
