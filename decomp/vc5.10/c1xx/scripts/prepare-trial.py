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


def compiler_support(candidate,indexes,accepted):
    """The encountered compiler import is support, never another app body."""
    support=candidate.get('support_imports',[])
    if support:
        if support!=['import:_ftol'] or candidate['trial']!='_calculate_arena_reservation' or support[0] in accepted:
            raise ValueError('not the bounded encountered automatic compiler _ftol support')
        record=indexes[support[0]]
        if (record['start'],record['end'],record['provider'])!=(0x10438f04,0x10438f0a,{'dll':'MSVCRT.dll','name':'_ftol'}):
            raise ValueError('compiler-support boundary identity changed')
    return support


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
    closure = candidate.get("ordered_closure")
    prospective = bool(candidate.get("prospective_only"))
    if prospective or closure:
        if len(cfg["accepted"]) != 57 or trial_name != '_initialize_frontend_37':
            raise ValueError('bounded PCH closure does not follow exact57-function prefix')
        wrapper, member = indexes['_initialize_frontend_37'], indexes['PchContext::Initialize']
        if (wrapper['start'],wrapper['end'],member['start']) != (0x10438ad8,0x10438ae2,0x1043bd41):
            raise ValueError('bounded PCH wrapper/member identity changed')
        if prospective:
            if closure or not member.get('pending_extent') or member['end'] != member['start']+1:
                raise ValueError('prospective member must remain genuine pending stub')
        elif closure != ['_initialize_frontend_37','PchContext::Initialize'] or member.get('pending_extent') or member['end'] != 0x1043bd53:
            raise ValueError('ordered PCH closure differs from authorized10+18-byte pair')
    support = compiler_support(candidate,indexes,cfg['accepted'])
    accepted = list(cfg["accepted"]) + (closure or ([trial_name] if trial_name else [])) + support
    base = cfg["target"]["base"]
    expected = []
    for name in accepted:
        record = indexes[name]
        for n, (first, last) in enumerate([(record["start"], record["end"])] +
                                          [tuple(pair) for pair in record.get("code_chunks", [])]):
            expected.append({"owner": name, "start_rva": first - base,
                             "end_rva": last - base, "entry": int(n == 0)})
        for first,last in record.get('shared',[]):
            expected.append({'owner':name,'start_rva':first-base,'end_rva':last-base,'entry':0,'kind':'text_data'})
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
    if support:trial['support_imports']=support
    if prospective:trial["prospective_only"] = True
    if closure:trial["ordered_closure"] = closure
    trial["ranges"] = expected
    trial["case"] = candidate["case"]
    trial["candidate_sha256"] = sha(PROJECT / candidate_path)
    trial["bound_inputs"] = {item: sha(PROJECT / item) for item in (
        "functions.json", "runtime.json", "graph-metadata.json", "build/runtime.json",
        "build/c1n32_l.dll", "build/c1xx.map", "build/native/c1xx-host")}
    if trial["case"] == "minimal":
        trial["case_inputs"] = {item: sha(PROJECT / item) for item in (
            "cases/minimal/contract.json", "cases/minimal/invoke-argv.bin", "cases/sources/minimal.cpp", "cases/minimal/state-model.json")}
    if json.loads((PROJECT/"graph-metadata.json").read_text()).get("register_imports"):
        trial["case_inputs"]["cases/minimal/allocation-preservation-v1.json"]=sha(PROJECT/"cases/minimal/allocation-preservation-v1.json")
    trial["test_seed"] = candidate.get("test_seed")
    trial['test_constructor_seed']=bool(candidate.get('test_constructor_seed'))
    if trial["test_seed"] or '_initialize_compiler_pass' in accepted:
        trial["validation_observers"] = [{"owner":"_initialize_compiler_pass", "original_rva":0x385be,
            "original_byte":0x89,"purpose":"observe typed copy/restore before first-flag reset"}]
    if trial_name=='CompilerPool::Initialize':
        trial['validation_observers'].append({'owner':'_initialize_frontend_24','original_rva':0x389d4,'original_byte':0xc3,'purpose':'actual pool member return-this and RET4 stack effect'})
    if '_reserve_work_buffer' in accepted:
        trial['allocation_observers']={'owner':'_reserve_work_buffer','before':[0x38cdf,0x38cf8],'after':[0x38ce1,0x38cfa],'helper_return':'actual captured stack return, only two declared callers','provider_iat':0xc9204,'final_bss_rva':0x38d07,'scope':'validation only; no extra unmask, stub-return or API mock'}
    if '_initialize_intermediate_files' in accepted:
        trial['early_options_observers']={'owner':'_initialize_intermediate_files','entry_rva':0x38d07,'frontier_rva':0x38d56,'purpose':'actual argv entry and first option/state ABI; full original BSS at frontier; no mocked inputs or returns'}
    if '_process_early_option' in accepted:
        trial['early_switch_observers']={'owner':'_process_early_option','scope':'25 real argv calls and actual returns; no environment arguments mocked','entry_rva':0x38d56,'return_rva':0x38d28,'shared_text_data_rvas':[[0x38de0,0x38df0]]}
    if '_initialize_frontend_hook' in accepted:
        trial['system_info_observers']={'owner':'_main','before_rva':0x38c42,'after_rva':0x38c48,'provider_iat_rva':0xc91f4,'destination_rva':0xcd7b8,'output_bytes':36,'scope':'one actual GetSystemInfo; no GlobalMemoryStatus or fabricated fields'}
    if '_calculate_arena_reservation' in accepted:
        trial['scaler_observers']={'owner':'_calculate_arena_reservation','original_points':[0x38ed7,0x38edd,0x38ee3,0x38ee9,0x38eee,0x38e81,0x38ea9,0x38eaf],'ftol_thunk_rva':0x38f04,'ftol_iat_rva':0xc90c8,'provider_export':'MSVCRT.dll!_ftol','provider_bytes':39,'x87_entry_profile':[0x027f,0x0020,0xffff],'scope':'actual math/conversion/QWORD return and single real TOP_DOWN reservation; no mock or code substitution'}
    if 'CompilerArena::Reset' in accepted:
        trial['scaler_observers'].update(calls=9,scope='nine ordered actual descriptor/calculator/API/Reset calls; persistent observer points; no code or API mock')
        trial['arena_reset_observers']={'entry_rva':0x38f0a,'return_rva':0x38ec8,'calls':9,'receiver_rvas':[0xc6500+i*32 for i in range(9)],'return_semantics':'void; caller overwrites EAX','full_original_bss_guard':True}
    if 'CompilerArena::CommitExtent' in accepted:
        trial['pool_cycle_observers']={'rvas':[0x1d76e,0x1d774,0x1ee9f,0x1ee52,0x38faa]+[0x38f27+i*12 for i in range(10)],'pool_order':[9,8,0,1,3,4,5,2,6,7],'calls':10,'commit_API_calls':1,'committed_bytes':65536,'scope':'exact source-owned return/API observations; no extra unmask, helper mock or region-tail reads'}
    if '_process_compiler_options' in accepted:
        trial['options_parser_observers']={'entry_rva':0x38fad,'first_call_rva':0x38fb9,'frontier_rva':0x2b834,'frontier_stack_arguments':[0],'rules':[0xd6160,101,12],'letters':[0xda388,12,12],'warnings':[0xd6628,11,8],'scope':'actual first named helper ABI and encountered loaded grammar/default records; no provider execution or mock'}
        for name in ('options-analysis.json','options-preservation-v1.json','options-preservation-ablation.json','mspdb-signature-boundary.json'):
            path='cases/minimal/'+name;trial['case_inputs'][path]=sha(PROJECT/path)
    if '_push_declaration_context' in accepted:
        trial['language_context_observers']={'entry_rva':0x2b834,'caller_return_rva':0x38fbe,'argument':0,'depth_rva':0xcd580,'slots_rva':0xc13d0,'slots':10,'scope':'scalar language context; actual first NULL-independent zero push, no mocked call'}
        if trial.get('test_seed'):trial['language_context_auxiliary']={'initializer_live_depth':7,'initializer_saved_depth':3,'initializer_slots':[1+(i&1)for i in range(10)],'helper_entry_slots':[1+(i&1)for i in range(10)],'helper_argument_unchanged':0,'code_or_helper_mock':False}
    if '_initialize_include_aliases'in accepted:
        trial['alias_root_observers']={'entry_rva':0x39282,'return_rva':0x38fcf,'request_bytes':4,'arena':0,'pointee_layout':'UNMODELED; bytes untouched','source_formals':[],'ABI':'cdecl0/RET0'}
    if '_initialize_checksums'in accepted:
        trial['checksum_observers']={'entry_rva':0x392a8,'end_rva':0x39394,'tables_rva':0xbe970,'table_bytes':768,'taps_rva':0xd5c58,'taps':[0,1,3,6,8,13,15,-1],'next_CRT_API':{'name':'setlocale','category':2,'locale':'','IAT_rva':0xc914c,'provider_export_rva':0x5729},'next_target_helper_rva':0x39394}
    if '_initialize_checksums'in accepted:
        trial['case_inputs']['cases/minimal/locale-schema.json']=sha(PROJECT/'cases/minimal/locale-schema.json')
    if '_initialize_lowercase_table' in accepted:
        trial['lowercase_observers']={'entry_rva':0x393ca,'return_rva':0x38fee,'IAT_rva':0xc9054,'public_entry_rva':0xa5d3,'provider_hot_bytes':54,'provider_case_table_rva':0x3f7f8,'provider_case_table_bytes':256,'target_map_rva':0xc13f8,'target_map_bytes':512,'next_rva':0x39413,'scope':'all actual unsigned-byte public calls/ABI/errors/fullFP and fullmap output; provider success unclaimed until reviewed'}
    if '_new_option_vector' in accepted:
        trial['vector_observers']={'entry_rva':0x682f,'calls':3,'arguments':[40,1],'header_bytes':12,'items_bytes':160,'arena_index':1,'committed_bytes':32768,'normal_frontier':'public:mspdb50!SigForPbCb','restore_frontier':'_next_argument','provider_execution':False}
    if trial_name in ('_next_argument','_match_option','_apply_option_action','_parse_option_integer','_match_option_suffix','_append_option_vector'):
        trial['argument_observers']={'entry_rva':0x39428,'end_rva':0x3944a,'return_rva':0x390b3,'next_rva':0x3944a,'scope':'restore-only actual Invoke26 to25, borrowed argv[1], real four-formal matcher; normal held at historical dependency entry'}
    if trial_name in ('_match_option','_apply_option_action','_parse_option_integer','_match_option_suffix','_append_option_vector'):
        trial['matcher_observers']={'entry_rva':0x3944a,'action_rva':0x39632,'next_callback_return_rva':0x3957b,'signature_return_rva':0x3909f,'original_table_rvas':[[0x395e8,0x39632]],'scope':'ordinary complete matcher candidate; own CPP Sig53B/table1024 pre/post then real apply action frontier, no standalone API-only role'}
    if trial_name=='_parse_option_integer' or '_parse_option_integer' in accepted:
        trial['case_inputs']['cases/minimal/integer-imports.json']=sha(PROJECT/'cases/minimal/integer-imports.json')
    if trial_name in ('_apply_option_action','_parse_option_integer','_match_option_suffix','_append_option_vector'):
        trial['option_action_observers']={'entry_rva':0x39632,'frontier_rva':0x39779,'matches':3,'actions':3,'action_returns':2,'outer_calls':3,'inner_calls':3,'signature_returns':[0x3909f,0x395af,0x39565],'scope':'complete own373B candidate; persistent indexed true argument/matcher/action/public entry+RET inventory, no response replay or per-API run'}
    if trial_name in ('_parse_option_integer','_match_option_suffix','_append_option_vector'):
        trial['option_action_observers'].update(frontier_rva=0x39757,matches=4,action_returns=3,outer_calls=4,scope='accepted373B action prefix continues through own numeric candidate to real suffix frontier')
        trial['integer_observers']={'entry_rva':0x39779,'return_rva':0x39694,'calls':1,'argument':'borrowed Invoke[6] 1 NUL','provider_names':['isspace','isdigit','atol'],'provider_rvas':[0x1c0be,0x1c065,0x5d30],'IAT_rvas':[0xc9154,0xc9090,0xc9000],'API_return_rvas':[0x39793,0x397a9,0x397bb],'frontier_rva':0x39757,'ctype_consumption':'zero versus nonzero only; raw provider masks retained separately;SBCS1 input49'}
    if trial_name in ('_match_option_suffix','_append_option_vector'):
        trial['option_action_observers'].update(frontier_rva=0x100ea,matches=5,actions=5,action_returns=4,outer_calls=5,scope='accepted numeric/action prefix continues through one real suffix call to append_option_vector')
        trial['suffix_observers']={'entry_rva':0x39757,'return_rva':0x394cf,'calls':1,'arguments':[{'rule':29,'Invoke_slot':7,'offset':2,'action':35}],'frontier_rva':0x100ea,'frontier_formals':['CompilerVector *vector','char *value'],'scope':'whole112B suffix candidate, one actual empty suffix; cold path source+graph only'}
    if trial_name=='_append_option_vector':
        trial['option_action_observers'].update(frontier_rva=0x397db,matches=21,actions=20,action_returns=20,outer_calls=21,inner_calls=5,scope='accepted option helpers through sole43B append candidate to genuine resolver')
        trial['integer_observers'].update(calls=5,argument='five actual borrowed numeric option spans; first Invoke[6] 1 NUL',inputs=['1','8','64','5','0'],frontier_rva=0x397db,ctype_consumption='zero versus nonzero only; raw provider masks retained separately;SBCS1 inputs49/56/54/53/48')
        trial['suffix_observers'].update(calls=3,arguments=[{'rule':29,'Invoke_slot':7,'offset':2,'action':35},{'rule':28,'Invoke_slot':10,'offset':3,'action':36},{'rule':48,'Invoke_slot':23,'offset':3,'action':34}],frontier_rva=0x397db,frontier_formals=['char *output','const char *input','unsigned capacity'],scope='three actual empty suffixes in accepted helper; whole43B append candidate')
        trial['append_observers']={'entry_rva':0x100ea,'return_rva':0x396bc,'calls':6,'vector_roots':['defines','defines','defines','defines','defines','include_directories'],'values':[{'Invoke_slot':8,'offset':2},{'Invoke_slot':11,'offset':2},{'Invoke_slot':12,'offset':2},{'Invoke_slot':18,'offset':2},{'Invoke_slot':19,'offset':2},{'Invoke_slot':25,'offset':0}],'grow_calls':0,'header_bytes':12,'items_bytes':160,'committed_bytes':32768,'frontier_rva':0x397db,'frontier_caller_rva':0x390f9,'frontier_formals':['char *output','const char *input','unsigned capacity'],'frontier_capacity':520,'scope':'six actual append entry/RET pairs, all original/native ordered options/public calls, no grow or resolver body credit'}
    if '_read_env_arguments' in accepted:
        trial['environment_argument_observers']={'entry_rva':0x39413,'return_rva':0x39021,'next_rva':0x682f,'argument':None,'next_arguments':[40,1],'scope':'actual NULL input/RET0/no state mutation, then owned caller warning writes and genuine allocation frontier; nonNULL path source+graph only'}
    if '_initialize_multibyte_categories' in accepted:
        trial['multibyte_observers']={'entry_rva':0x39394,'return_rva':0x38fe9,'public_entry_rva':0x2257,'IAT_rva':0xc9184,'data_IAT_rva':0xc90c4,'codepage':-4,'next_rva':0x393ca,'flags_rva':0xcc108,'classes_rva':0xcc008,'bytes':512,'selection_support':'unclaimed until own203 captured and reviewed'}
        trial['case_inputs']['cases/minimal/scanner-data.json']=sha(PROJECT/'cases/minimal/scanner-data.json')
    trial['test_arena_reset_seed']=bool(candidate.get('test_arena_reset_seed'))
    if trial['test_arena_reset_seed']:
        if trial_name!='CompilerArena::Reset':raise ValueError('Reset seed requires sole Reset candidate')
        trial['auxiliary_arena_reset_data_seed']={'point':'each actual Reset entry before body','cursor':'owned reserved base+4','committed_limit':'owned reserved base+8','committed_bytes':17,'reset_value':37,'other_fields_unchanged':True,'code_or_API_mock':False}
    trial['test_action_seed']=bool(candidate.get('test_action_seed'))
    if trial['test_action_seed']:
        trial['auxiliary_action_data_seed']={'owner':'_initialize_frontend_41','point':'actual entry after BSS reset','head':'owned snapshot+4','tail':'owned snapshot+8','depth':17,'pointees_dereferenced':False,'code_or_allocator_mock':False}
    trial['test_comment_seed']=bool(candidate.get('test_comment_seed'))
    if trial['test_comment_seed']:
        trial['auxiliary_comment_data_seed']={'owner':'_initialize_generated_comment','point':'actual constructor entry after BSS reset','com_initialized':37,'generated_comment':'74 bytes165','comment_value_offset':23,'generated_comment_length':29,'code_or_allocator_mock':False}
    trial['test_group_seed']=bool(candidate.get('test_group_seed'))
    if trial['test_group_seed']:
        trial['auxiliary_group_data_seed']={'owner':'_initialize_frontend_38','point':'actual entry after BSS reset','head':'owned snapshot+4','active':'owned snapshot+8','tail_untouched':True,'pointees_dereferenced':False,'code_or_allocator_mock':False}
    trial['test_pch_seed']=bool(candidate.get('test_pch_seed'))
    if trial['test_pch_seed']:
        if not closure:raise ValueError('PCH constructor data auxiliary requires declared joint closure')
        trial['auxiliary_pch_data_seed']={'owner':'PchContext::Initialize','point':'actual member entry after BSS reset','receiver_unchanged':True,'scalars':{'image_count':17,'stored_file_position':23,'file_offset':-11,'loaded_image_count':29},'pointer_fields':'owned snapshot interiors +4..+52, never dereferenced','code_or_allocator_mock':False}
    if 'PchContext::Initialize' in accepted:
        trial['validation_observers'].append({'owner':'_initialize_compiler_pass','original_rva':0x385f3,'original_byte':0xeb,'purpose':'actual PCH member return to inherited caller, RET0 and EDI preservation','activation':'armed only at actual member entry'})
    trial['test_prototype_seed']=bool(candidate.get('test_prototype_seed'))
    if trial['test_prototype_seed']:
        trial['auxiliary_prototype_data_seed']={'owner':'_initialize_frontend_36','point':'actual entry before target body, after BSS reset','bytes':20,'values':'1..20','code_or_allocator_mock':False}
    trial['test_pool_seed']=bool(candidate.get('test_pool_seed'))
    if trial['test_pool_seed']:
        trial['auxiliary_pool_data_seed']={'owner':'CompilerPool::Initialize','point':'actual member entry before body','cursor':'owned snapshot+4','remaining':23,'minimum_pages':37,'arena':'owned snapshot+8; never dereferenced','actual_this_and_argument_unchanged':True,'code_or_allocator_mock':False}
    trial['test_pdb_seed']=candidate.get('test_pdb_seed')
    if trial['test_pdb_seed'] is not None:
        if trial['test_pdb_seed'] not in (0,1):raise ValueError('unknown PDB reuse mode')
        trial['auxiliary_pdb_data_seed']={'point':'initializer entry before BSS reset; restore_mode overridden at PDB constructor entry','owner':trial_name,'pointer_fields':'owned snapshot interiors +0/+4/+8/+12/+16; never dereferenced','read_mode':37,'reuse_mode':trial['test_pdb_seed'],'code_or_allocator_mock':False}
    trial['test_token_seed']=candidate.get('test_token_seed')
    if trial['test_token_seed']:
        if trial['test_token_seed'] not in ('positive','negative'):raise ValueError('unknown token seed arm')
        trial['auxiliary_token_data_seed']={'owner':'_initialize_frontend_18','point':'entry before target body','arm':'integer','source':0x1122334455667788 if trial['test_token_seed']=='positive' else -0x112233445566778,'old_cache':-7,'old_kind':165,'unknown_tails_seeded':False,'pointer_arm_tested':False,'code_or_allocator_mock':False}
    trial['test_deferred_seed']=bool(candidate.get('test_deferred_seed'))
    if trial['test_deferred_seed']:
        trial['auxiliary_deferred_data_seed']={'owner':'_initialize_deferred_work','point':'entry before target body','deferred_work':'owned snapshot identity; never dereferenced','code_or_allocator_mock':False}
    if trial['test_constructor_seed']:
        trial['auxiliary_data_seed']={'owner':trial_name,'point':'entry before target body',
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
