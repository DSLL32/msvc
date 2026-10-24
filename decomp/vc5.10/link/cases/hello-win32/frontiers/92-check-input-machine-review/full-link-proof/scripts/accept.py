#!/usr/bin/env python3
"""Verify fresh candidate-in-prefix evidence before promoting one function."""
import argparse
import copy
import hashlib
import json
import os
import struct
from pathlib import Path
import subprocess
import time
from mask import BINARY, ROOT, construct


def sha(data): return hashlib.sha256(data).hexdigest()


def trial_for(current, candidate, support=()):
    assert candidate not in current['accepted'], 'candidate already accepted'
    records = current['driver'] + json.loads((BINARY / 'runtime.json').read_text())
    matches = [r for r in records if r['name'] == candidate]
    assert len(matches) == 1 and not matches[0].get('pending_extent'), 'unknown/pending candidate'
    assert matches[0].get('status') == 'implemented', 'candidate remains a stub'
    result = copy.deepcopy(current)
    result['accepted'].append(candidate)
    if support:
        assert candidate=='_free_buffer4' and list(support)==['import:??3@YAXPAX@Z'], 'unrelated support closure'
        import importlib
        checker=importlib.import_module('delete-support')
        for name in support:
            assert name not in current['accepted']
            record,=[record for record in records if record['name']==name]
            checker.check(record)
            result['accepted'].append(name)
        construct(result) # Reject any overlapping application/loader ownership.
    return result


def bind_capture(directory, expected, kind):
    receipt = json.loads((directory / 'receipt.json').read_text())
    config = (directory / 'functions.json').read_bytes()
    captured=json.loads(config)
    if kind=='original':
        assert captured['target']==expected['target'] and captured['accepted']==expected['accepted'], 'original capture is not the current trial prefix'
        case=receipt.get('case','help')
        assert captured['cases'][case]['arguments']==expected['cases'][case]['arguments']
        if case=='help':assert expected['cases'][case]['arguments']==['/?']
        else:
            from case_inputs import execution,load,windows_work
            assert execution(receipt['case_manifest'])==execution(load(case,expected))
            assert receipt['working_directory']==windows_work(case)
    else:
        assert captured == expected, 'captured configuration is not current trial'
        assert config == (json.dumps(expected, indent=2) + '\n').encode(), 'captured trial configuration bytes differ'
    assert sha(config) == receipt['configuration_sha256'], 'captured configuration hash differs'
    assert (directory / 'runtime.json').read_bytes() == (BINARY / 'runtime.json').read_bytes(), 'stale runtime index'
    for name, record in receipt['files'].items():
        data = (directory / name).read_bytes()
        assert len(data) == record['bytes'] and sha(data) == record['sha256'], 'capture file changed: ' + name
    if kind!='original':
        case=receipt.get('case','help')
        if case!='help':
            from case_inputs import execution,load,windows_work
            assert execution(receipt['case_manifest'])==execution(load(case,expected))
            assert receipt['working_directory']==windows_work(case)
        assert receipt['native_sha256'] == sha((BINARY / 'build/native/LINK.EXE').read_bytes()), 'stale native executable'
        assert receipt['rebuilt_sha256'] == sha((BINARY / 'build/link.exe').read_bytes()), 'stale rebuilt executable'
        if (directory/'compiled').exists():
            assert sha((directory/'compiled/native-LINK.EXE').read_bytes())==receipt['native_sha256'], 'frozen native executable differs'
            assert sha((directory/'compiled/original-toolchain-link.exe').read_bytes())==receipt['rebuilt_sha256'], 'frozen rebuilt executable differs'
            assert (directory/'compiled/original-toolchain-link.map').read_bytes()==(BINARY/'build/link.map').read_bytes(), 'frozen rebuilt map differs'
        sources = {str(p.relative_to(BINARY)):sha(p.read_bytes()) for p in sorted(p for p in (BINARY/'src').iterdir() if p.suffix in ('.c','.h','.cpp'))}
        assert receipt['source_files'] == sources, 'compiled sources changed after capture'
    for name,value in receipt.get('source_content',{}).items():
        assert sha((directory/'source'/name).read_bytes())==value, 'frozen source content changed'
    if kind == 'original':
        assert receipt['mode'] == 'masked' and receipt['helper_status'] == 0
        assert receipt['actual_loaded_crt_verified'] and receipt['actual_loaded_target_hfile_verified']
        assert receipt['original_sha256'] == expected['target']['sha256']
        assert sha((ROOT / expected['target']['path']).read_bytes()) == expected['target']['sha256']
        assert sha((directory / 'module-000.bin').read_bytes()) == expected['target']['sha256'], 'actual target hFile changed'
        # A validator/debugger build update does not change this captured target
        # execution. Its original helper identity remains in the frozen receipt.
        crt_hash = 'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210'
        assert receipt['actual_loaded_crt_hfile_copies'], 'no actual CRT module hFile'
        for name in receipt['actual_loaded_crt_hfile_copies']:
            data = (directory / name).read_bytes()
            assert name.startswith('module-') and len(data) == 277776 and sha(data) == crt_hash, 'actual CRT module differs'
        mask, _ = construct(expected)
        mutable = bytearray(mask)
        mask_rva = int.from_bytes(mask[:4], 'little')
        records = expected['driver'] + json.loads((BINARY / 'runtime.json').read_text())
        expected_entries = []
        for record in records:
            if record['name'] in expected['accepted']:
                rva = record['start'] - 0x400000
                position = 8 + rva - mask_rva
                expected_entries.append('%x %x 0 %s\n' % (rva, mutable[position], record['name']))
                mutable[position] = 0xcc
        from observers import plan
        for rva,name in plan(expected['accepted'],receipt.get('case','help')):
            position=8+rva-mask_rva
            expected_entries.append('%x %x 0 %s\n'%(rva,mutable[position],name));mutable[position]=0xcc
        mask = bytes(mutable)
        assert (directory / 'entries.txt').read_text() == ''.join(expected_entries), 'entry trace plan changed'
        assert (directory / 'mask.bin').read_bytes() == mask and receipt['mask_sha256'] == sha(mask)
    else:
        assert receipt['kind'] == ('native-i386-completion'if kind=='native-complete'else 'native-i386-gdb-frontier') and receipt['status'] == 0
    return receipt


def verify_frontier(original, native, expected_name, expected):
    assert expected_name, 'nonempty expected frontier is mandatory'
    original_receipt=json.loads((original/'receipt.json').read_text());native_receipt=json.loads((native/'receipt.json').read_text())
    assert original_receipt.get('case','help')==native_receipt.get('case','help')
    if original_receipt.get('case','help')!='help':
        def artifacts(directory):return {p.name:p.read_bytes()for p in (directory/'artifacts').iterdir()}
        assert artifacts(original)==artifacts(native),'partial generated artifacts differ'
    events = [json.loads(line) for line in (original / 'events.jsonl').read_text().splitlines()]
    assert events[-1] == dict(event='summary', failed=0, status=999), 'original did not stop cleanly'
    traps = [event for event in events if event['event'] == 'trap']
    assert len(traps) == 1, 'original must have exactly one terminal trap'
    records = expected['driver'] + json.loads((BINARY / 'runtime.json').read_text())
    if expected_name == '_mainCRTStartup' and not expected['accepted']:
        record = dict(start=0x465280)
    else:
        record, = [record for record in records if record['name'] == expected_name]
    assert traps[0]['address'] == record['start'], 'wrong original frontier'
    state = json.loads((native / 'state.json').read_text())
    native_name = record.get('native_symbol',{'_mainCRTStartup':'target_startup', '_main':'link_main'}.get(expected_name, expected_name.lstrip('_')))
    assert state.get('signal_number') == 5 and state.get('previous_byte') == 204 and state['function'] == native_name, 'wrong native frontier'
    for name in ['stdout.bin', 'stderr.bin']:
        assert (original / name).read_bytes() == (native / name).read_bytes(), 'partial stream differs: ' + name
    applications={record['name']for record in expected['driver']}
    original_entries=[event['name']for event in events if event['event']=='entry'and event['name']in applications]
    assert original_entries==state['entries'],'application entry prefix differs'
    # State comparison is frontier-specific and reads raw debugger dumps. Until
    # that contract exists, a matching address/stream alone cannot accept code.
    from boundary_state import compare
    compare(expected_name, original, native, traps[0], state)
    if state.get('system_calls'):
        binding=json.loads((native/'environment/binding.json').read_text())
        assert binding['reference_sha256']==sha((original/'receipt.json').read_bytes()), 'native environment belongs to a different original sample'
        assert (native/'environment/system-after.bin').read_bytes()==(original/'system-after.bin').read_bytes()
        assert (native/'environment/memory-after.bin').read_bytes()==(original/'memory-after.bin').read_bytes()


def verify_completion(original,native,expected,native_state=None):
    import importlib
    replay=importlib.import_module('check-native')
    master=replay.completed_master(original,expected)
    state=json.loads((native/'state.json').read_text())if native_state is None else native_state
    receipt=json.loads((native/'receipt.json').read_text())
    assert receipt['kind']=='native-i386-completion'
    reference_sha=sha((original/'receipt.json').read_bytes())
    assert receipt['reference_receipt_sha256']==reference_sha
    assert json.loads((native/'environment/binding.json').read_text())['reference_sha256']==reference_sha
    assert state['logical_status']==master['status']==1100
    assert state['posix_status']==master['posix_status']==76
    raw=bytes.fromhex(state['terminal_record_hex']);assert len(raw)==24
    magic,version,owner,status,code,reserved=struct.unpack('<6I',raw)
    assert (magic,version,status,code,reserved)==(0x31535452,1,1,1100,0)
    assert owner==state['native_pid'] and state['posix_status']==(code&255)
    assert state['entries']==master['application_trace']
    for name in ['stdout.bin','stderr.bin']:
        assert (original/name).read_bytes()==(native/name).read_bytes()
    return master


def audit_trial(path):
    audit_started=time.perf_counter()
    tool = Path(os.environ.get('GRAPH_AUDIT_BIN',str(ROOT / 'tooling/graph-audit/target/release/graph-audit'))).resolve()
    command = [str(tool), '--profile', 'link', '--discovery', 'cfg', '--repo', str(ROOT), '--project', str(BINARY),
               '--original', str(ROOT / 'toolchains/5.10/bin/LINK.EXE'), '--rebuilt', str(BINARY / 'build/link.exe'),
               '--map', str(BINARY / 'build/link.map'), '--config', str(path),
               '--runtime', str(BINARY / 'runtime.json'), '--rebuilt-runtime', str(BINARY / 'build/runtime.json'),
               '--metadata', str(BINARY / 'build/graph.json'), '--check-ret', '--check-references',
               '--check-ownership', '--check-extents', '--check-callbacks']
    trial = json.loads(path.read_text())
    runtime = json.loads((BINARY/'runtime.json').read_text())
    import_leaves = {record['name'] for record in runtime if record.get('classification')=='import'}
    strict = copy.deepcopy(trial)
    strict['accepted'] = [name for name in strict['accepted'] if name not in import_leaves]
    strict_path = path.parent/'strict-ret-config.json'
    strict_path.write_text(json.dumps(strict,indent=2)+'\n')
    inputs = [tool, ROOT/'toolchains/5.10/bin/LINK.EXE', BINARY/'build/link.exe',
              BINARY/'build/link.map',path,BINARY/'runtime.json',BINARY/'build/runtime.json',BINARY/'graph.json',BINARY/'build/graph.json']
    inputs.append(strict_path)
    inputs += [BINARY/'scripts'/name for name in ['accept.py','boundary_state.py','hello_boundary.py','mask.py','observers.py','environment_profile.py','binary.py','context_heap.py','section_state.py','group_state.py','collision_state.py','file_open_state.py','file_move_state.py','file_mapping_state.py','mapping_completion_state.py','library_creation_state.py','input_open_state.py','archive_state.py','mapped_seek_state.py','big_endian_state.py','swap_result_state.py','input_machine_state.py','detect_machine_state.py','mapped_input_contract.py','capture_sources.py','native-completion.py','check-native.py','completion_cleanup.py','case_inputs.py']]
    inputs.append(BINARY/'cases/hello-win32/mapped-open-byte-input.json')
    inputs.append(BINARY/'cases/hello-win32/input-loader-contract.json')
    inputs.append(BINARY/'cases/hello-win32/input-reader-contract.json')
    inputs.extend([BINARY/'cases/hello-win32/archive-signature-literal.json',BINARY/'cases/hello-win32/mapped-read-output-contract.json'])
    inputs.extend([BINARY/'cases/hello-win32/mapped-read-contract.json',BINARY/'cases/hello-win32/mac-resource-input-contract.json',BINARY/'cases/hello-win32/mac-resource-contract.json',BINARY/'cases/hello-win32/file-length-input-contract.json',BINARY/'cases/hello-win32/file-length-contract.json',BINARY/'cases/hello-win32/seek-file-input-contract.json',BINARY/'cases/hello-win32/seek-file-contract.json',BINARY/'cases/hello-win32/mapped-seek-input-contract.json',BINARY/'cases/hello-win32/mapped-seek-contract.json',BINARY/'cases/hello-win32/big-endian-input-contract.json',BINARY/'cases/hello-win32/big-endian-contract.json',BINARY/'cases/hello-win32/swap-dword-input-contract.json',BINARY/'cases/hello-win32/object-reader-input-contract.json',BINARY/'cases/hello-win32/input-machine-input-contract.json',BINARY/'cases/hello-win32/detect-machine-input-contract.json'])
    cached_bodies={'_main','_initialize_debug@8','_initialize_repro','_parse_command_line@16','_has_switch@4'} & set(trial['accepted'])
    imported=copy.deepcopy(trial)
    register_candidates=[name for name in ['_configure_link_options@12','_parse_switch@8','_print_logo']if name in trial['accepted']]
    register_candidate=bool(register_candidates)
    full_link_candidate='?FullLink@LinkContext@@QAEHPAE@Z' in trial['accepted']
    pass_one_candidate='LinkContext::PassOne'in trial['accepted']
    collision_candidate='_check_output_input_collision@8'in trial['accepted']
    mapping_candidate='_initialize_file_mapping@12'in trial['accepted']
    finder_candidate='_find_library@12'in trial['accepted']
    imported['accepted']=[name for name in imported['accepted'] if name not in cached_bodies and name not in register_candidates and not(full_link_candidate and name=='?FullLink@LinkContext@@QAEHPAE@Z')and not(pass_one_candidate and name=='LinkContext::PassOne')and not(collision_candidate and name=='_check_output_input_collision@8')and not(mapping_candidate and name=='_initialize_file_mapping@12')and not(finder_candidate and name=='_find_library@12')]
    import_path=path.parent/'direct-import-config.json'
    import_path.write_text(json.dumps(imported,indent=2)+'\n')
    inputs.append(import_path)
    if cached_bodies:inputs += [BINARY/'scripts/import-residual.py',BINARY/'build/driver.obj',BINARY/'build/driver-stubs.obj',BINARY/'build/reproduction.obj',BINARY/'build/arguments.obj']
    if 'LinkContext::Initialize' in trial['accepted']:inputs.append(BINARY/'scripts/member_pointers.py')
    register_command=None
    register_packet_check=None
    if register_candidate:
        import importlib
        from binary import PE
        witness=importlib.import_module('options-preservation')
        register_packet_check=witness.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'))
        packet=json.loads(witness.PACKET.read_text())
        register_config=copy.deepcopy(trial)
        register_config['accepted']=register_candidates
        register_config_path=path.parent/'register-import-config.json'
        register_config_path.write_text(json.dumps(register_config,indent=2)+'\n')
        register_metadata=json.loads((BINARY/'build/graph.json').read_text())
        for name in register_candidates:
            register_metadata['functions'].setdefault(name,{})['register_imports']={'require_all_register_calls_resolved':True}
        contracts=[]
        for imported_api in packet['imports']:
            dll,name=imported_api['identity'].split('!',1)
            contracts.append({'target':{'import':{'dll':dll,'name':name}}})
            contracts[-1].update(preserves=['ebx','esi','edi'],evidence=dict(path='cases/help/options-import-preservation.json',sha256=sha(witness.PACKET.read_bytes())))
        if '_parse_switch@8' in register_candidates:
            parser_path=BINARY/'cases/help/parser-import-preservation.json'
            parser_packet=json.loads(parser_path.read_text())
            witness.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),parser_packet,{'strchr'})
            contracts.append({'target':{'import':{'dll':'msvcrt.dll','name':'strchr'}},'preserves':['ebx','esi','edi'],'evidence':{'path':'cases/help/parser-import-preservation.json','sha256':sha(parser_path.read_bytes())}})
            witness.check_parser_helper(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text())
            contracts.append({'target':{'function':'_link_fatal'},'preserves':['ebx','esi','edi'],'evidence':{'path':'cases/help/parser-import-preservation.json','sha256':sha(parser_path.read_bytes())}})
            inputs.append(parser_path)
            inputs += [ROOT/d['path']for api in parser_packet['imports']for d in api['declarations']]
        if '_print_logo' in register_candidates:
            logo_path=BINARY/'cases/help/logo-import-preservation.json'
            logo_packet=json.loads(logo_path.read_text())
            witness.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),logo_packet,{'printf'})
            witness.check_logo_helper(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text())
            for target in [{'import':{'dll':'msvcrt.dll','name':'printf'}},{'function':'_append_bytes@12'}]:
                contracts.append(dict(target=target,preserves=['ebx','esi','edi'],evidence=dict(path='cases/help/logo-import-preservation.json',sha256=sha(logo_path.read_bytes()))))
            inputs.append(logo_path)
            inputs += [ROOT/d['path']for api in logo_packet['imports']for d in api['declarations']]
        register_metadata['register_imports']={'preservation_contracts':contracts}
        register_metadata_path=path.parent/'register-import-metadata.json'
        register_metadata_path.write_text(json.dumps(register_metadata,indent=2)+'\n')
        register_command=command.copy()
        register_command[register_command.index('--config')+1]=str(register_config_path)
        register_command[register_command.index('--metadata')+1]=str(register_metadata_path)
        register_command+=['--check-imports','--resolve-register-imports']
        inputs += [register_config_path,register_metadata_path,witness.PACKET,BINARY/'scripts/options-preservation.py']
        provider=packet['canonical_provider']
        inputs += [ROOT/provider['path'],BINARY/provider['owned_receipt']]
        inputs += [BINARY/item['path'] for item in provider['owned_hfile_copies']]
        inputs += [ROOT/d['path'] for api in packet['imports'] for d in api['declarations']]
    full_link_command=None
    full_link_witness=None
    if full_link_candidate:
        import importlib
        from binary import PE
        full_link_checker=importlib.import_module('full-link-preservation')
        full_link_witness=full_link_checker.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),
            PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text(),trial)
        full_link_template=BINARY/'cases/hello-win32/full-link-audit-trial/metadata.json'
        full_link_metadata=json.loads(full_link_template.read_text())
        full_link_metadata_path=path.parent/'full-link-register-import-metadata.json'
        full_link_metadata_path.write_text(json.dumps(full_link_metadata,indent=2)+'\n')
        full_link_config=copy.deepcopy(trial)
        full_link_config['accepted']=['?FullLink@LinkContext@@QAEHPAE@Z']
        full_link_config_path=path.parent/'full-link-register-import-config.json'
        full_link_config_path.write_text(json.dumps(full_link_config,indent=2)+'\n')
        full_link_command=command.copy()
        full_link_command[full_link_command.index('--config')+1]=str(full_link_config_path)
        full_link_command[full_link_command.index('--metadata')+1]=str(full_link_metadata_path)
        full_link_command+=['--check-imports','--resolve-register-imports']
        inputs += [full_link_config_path,full_link_metadata_path,full_link_template,
            full_link_checker.PACKET,BINARY/'scripts/full-link-preservation.py',
            BINARY/'cases/hello-win32/full-link-closed-slot.json',
            BINARY/'cases/hello-win32/full-link-guard-controls/validation.json',
            BINARY/'cases/hello-win32/full-link-guard-controls/compile.json',
            BINARY/'cases/hello-win32/full-link-guard-controls/data-alias.obj']
        full_link_packet=json.loads(full_link_checker.PACKET.read_text())
        inputs += [BINARY/item['path']for item in full_link_packet['canonical_provider']['owned_hfile_copies']]
        inputs += [ROOT/item['path']for api in full_link_packet['imports']for item in api['declarations']]
        tick=full_link_packet['get_tick_count_contract']
        inputs += [BINARY/tick['hfile']['path'],BINARY/tick['hfile']['receipt'],ROOT/tick['declaration']['path']]
        inputs += sorted((BINARY/'build').glob('*.obj'))
    pass_one_command=None
    pass_one_witness=None
    if pass_one_candidate:
        import importlib
        from binary import PE
        pass_one_checker=importlib.import_module('pass-one-preservation')
        pass_one_witness=pass_one_checker.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),
            PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text())
        pass_one_config=copy.deepcopy(trial)
        pass_one_config['accepted']=['LinkContext::PassOne']
        pass_one_config_path=path.parent/'pass-one-register-import-config.json'
        pass_one_config_path.write_text(json.dumps(pass_one_config,indent=2)+'\n')
        pass_one_metadata=json.loads((BINARY/'build/graph.json').read_text())
        pass_one_metadata['functions'].setdefault('LinkContext::PassOne',{})['register_imports']={'require_all_register_calls_resolved':True}
        evidence=dict(path='cases/hello-win32/pass-one-import-preservation.json',sha256=sha(pass_one_checker.PACKET.read_bytes()))
        pass_one_metadata['register_imports']={'preservation_contracts':[
            dict(target=dict(function='_link_message'),preserves=['esi'],evidence=evidence),
            dict(target={'import':dict(dll='msvcrt.dll',name='fputc')},preserves=['esi'],evidence=evidence)]}
        pass_one_metadata_path=path.parent/'pass-one-register-import-metadata.json'
        pass_one_metadata_path.write_text(json.dumps(pass_one_metadata,indent=2)+'\n')
        pass_one_command=command.copy()
        pass_one_command[pass_one_command.index('--config')+1]=str(pass_one_config_path)
        pass_one_command[pass_one_command.index('--metadata')+1]=str(pass_one_metadata_path)
        pass_one_command+=['--check-imports','--resolve-register-imports']
        inputs += [pass_one_config_path,pass_one_metadata_path,pass_one_checker.PACKET,BINARY/'scripts/pass-one-preservation.py']
        packet=json.loads(pass_one_checker.PACKET.read_text());provider=packet['canonical_provider']
        inputs += [ROOT/provider['path'],BINARY/provider['owned_receipt']]
        inputs += [BINARY/item['path']for item in provider['owned_hfile_copies']]
        inputs += [ROOT/item['path']for imported in packet['imports']for item in imported['declarations']]
    collision_command=None
    collision_witness=None
    if collision_candidate:
        import importlib
        from binary import PE
        collision_checker=importlib.import_module('collision-preservation')
        collision_witness=collision_checker.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),
            PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text())
        collision_config=copy.deepcopy(trial);collision_config['accepted']=[collision_checker.OWNER]
        collision_config_path=path.parent/'collision-register-import-config.json'
        collision_config_path.write_text(json.dumps(collision_config,indent=2)+'\n')
        collision_metadata=json.loads((BINARY/'build/graph.json').read_text())
        collision_metadata['functions'].setdefault(collision_checker.OWNER,{})['register_imports']={'require_all_register_calls_resolved':True}
        collision_metadata['register_imports']={'preservation_contracts':collision_checker.contracts()}
        collision_metadata_path=path.parent/'collision-register-import-metadata.json'
        collision_metadata_path.write_text(json.dumps(collision_metadata,indent=2)+'\n')
        collision_command=command.copy()
        collision_command[collision_command.index('--config')+1]=str(collision_config_path)
        collision_command[collision_command.index('--metadata')+1]=str(collision_metadata_path)
        collision_command+=['--check-imports','--resolve-register-imports']
        inputs += [collision_config_path,collision_metadata_path,collision_checker.PACKET,BINARY/'scripts/collision-preservation.py']
        packet=json.loads(collision_checker.PACKET.read_text());provider=packet['canonical_provider']
        inputs += [ROOT/provider['path'],BINARY/provider['owned_receipt']]
        inputs += [BINARY/item['path']for item in provider['owned_hfile_copies']]
        inputs += [ROOT/item['path']for imported in packet['imports']for item in imported['declarations']]
    mapping_command=None
    mapping_witness=None
    if mapping_candidate:
        import importlib
        from binary import PE
        mapping_checker=importlib.import_module('mapping-preservation')
        mapping_witness=mapping_checker.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'))
        mapping_config=copy.deepcopy(trial);mapping_config['accepted']=[mapping_checker.OWNER]
        mapping_config_path=path.parent/'mapping-import-config.json';mapping_config_path.write_text(json.dumps(mapping_config,indent=2)+'\n')
        mapping_metadata=json.loads((BINARY/'build/graph.json').read_text())
        mapping_metadata['functions'].setdefault(mapping_checker.OWNER,{})['register_imports']={'require_all_register_calls_resolved':True}
        mapping_metadata['register_imports']={'preservation_contracts':mapping_checker.contracts()}
        mapping_metadata_path=path.parent/'mapping-import-metadata.json';mapping_metadata_path.write_text(json.dumps(mapping_metadata,indent=2)+'\n')
        mapping_command=command.copy();mapping_command[mapping_command.index('--config')+1]=str(mapping_config_path);mapping_command[mapping_command.index('--metadata')+1]=str(mapping_metadata_path)
        mapping_command+=['--check-imports','--resolve-register-imports']
        inputs += [mapping_config_path,mapping_metadata_path,mapping_checker.PACKET,BINARY/'scripts/mapping-preservation.py']
        packet=json.loads(mapping_checker.PACKET.read_text());provider=packet['provider']
        inputs += [BINARY/provider['path'],BINARY/provider['receipt']]
        inputs += [ROOT/d['path'] for d in packet['declarations']]
    finder_command=None
    finder_witness=None
    if finder_candidate:
        import importlib
        from binary import PE
        finder_checker=importlib.import_module('library-finder-preservation')
        finder_witness=finder_checker.check(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text())
        finder_config=copy.deepcopy(trial);finder_config['accepted']=[finder_checker.OWNER]
        finder_config_path=path.parent/'finder-import-config.json';finder_config_path.write_text(json.dumps(finder_config,indent=2)+'\n')
        finder_metadata=json.loads((BINARY/'build/graph.json').read_text())
        finder_metadata['functions'].setdefault(finder_checker.OWNER,{})['register_imports']={'require_all_register_calls_resolved':True}
        finder_metadata['register_imports']={'preservation_contracts':finder_checker.contracts()}
        finder_metadata_path=path.parent/'finder-import-metadata.json';finder_metadata_path.write_text(json.dumps(finder_metadata,indent=2)+'\n')
        finder_command=command.copy();finder_command[finder_command.index('--config')+1]=str(finder_config_path);finder_command[finder_command.index('--metadata')+1]=str(finder_metadata_path)
        finder_command+=['--check-imports','--resolve-register-imports']
        inputs += [finder_config_path,finder_metadata_path,finder_checker.PACKET,BINARY/'scripts/library-finder-preservation.py']
        packet=json.loads(finder_checker.PACKET.read_text());provider=packet['canonical_provider']
        inputs += [BINARY/provider['owned_receipt']]+[BINARY/item['path']for item in provider['owned_hfile_copies']]
        inputs += [ROOT/item['path']for imported in packet['imports']for item in imported['declarations']]
    delete_support=None
    if 'import:??3@YAXPAX@Z' in trial['accepted']:
        import importlib
        checker=importlib.import_module('delete-support')
        record,=[r for r in runtime if r['name']==checker.NAME]
        delete_support=checker.check(record)
        inputs += [BINARY/'scripts/delete-support.py',checker.PROOF,BINARY/'build/operator-delete-alias.obj']
        inputs += [file for directory in ['operator-delete/controlled-probe','operator-delete/first-target']for file in (BINARY/'cases/help'/directory).iterdir()if file.is_file()]
    before = {str(p):sha(p.read_bytes()) for p in inputs}
    full_command = [arg for arg in command if arg!='--check-ret'] + ['--include-evidenced-runtime']
    started=time.perf_counter()
    full = subprocess.run(full_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    strict_command = command.copy()
    strict_command[strict_command.index('--config')+1] = str(strict_path)
    strict_command.append('--include-evidenced-runtime')

    result = subprocess.run(strict_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    import_command=full_command.copy()
    import_command[import_command.index('--config')+1]=str(import_path)
    import_command.append('--check-imports')
    checked_imports=subprocess.run(import_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    residual=None
    if cached_bodies:
        import importlib
        from binary import PE
        module=importlib.import_module('import-residual')
        residual={}
        original_pe=PE(ROOT/'toolchains/5.10/bin/LINK.EXE');rebuilt_pe=PE(BINARY/'build/link.exe')
        map_text=(BINARY/'build/link.map').read_text()
        if '_main' in cached_bodies:residual['_main']=module.check(original_pe,rebuilt_pe,(BINARY/'build/driver.obj').read_bytes(),map_text)
        if '_initialize_debug@8' in cached_bodies:residual['_initialize_debug@8']=module.check_debug(original_pe,rebuilt_pe,(BINARY/'build/driver-stubs.obj').read_bytes(),map_text)
    if '_initialize_repro' in cached_bodies:residual['_initialize_repro']=module.check_repro(original_pe,rebuilt_pe,(BINARY/'build/reproduction.obj').read_bytes(),map_text)
    if '_parse_command_line@16' in cached_bodies:residual['_parse_command_line@16']=module.check_commandline(original_pe,rebuilt_pe,(BINARY/'build/arguments.obj').read_bytes(),map_text)
    if '_has_switch@4' in cached_bodies:residual['_has_switch@4']=module.check_switch(original_pe,rebuilt_pe,(BINARY/'build/arguments.obj').read_bytes(),map_text)
    register_audit=None
    if register_command:
        resolved=subprocess.run(register_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
        register_report=json.loads(resolved.stdout)
        assert set(register_report['functions'])==set(register_candidates)
        from binary import PE
        register_audit=dict(command=register_command,packet_binding=register_packet_check,
            inventory=witness.check_resolution(register_report,PE(BINARY/'build/link.exe')),
            report=register_report,stderr=resolved.stderr.decode())
    full_link_audit=None
    if full_link_command:
        resolved=subprocess.run(full_link_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
        full_link_report=json.loads(resolved.stdout)
        assert set(full_link_report['functions'])=={'?FullLink@LinkContext@@QAEHPAE@Z'}
        full_link_audit=dict(command=full_link_command,witness_binding=full_link_witness,
            inventory=full_link_checker.check_resolution(full_link_report,PE(BINARY/'build/link.exe')),
            report=full_link_report,stderr=resolved.stderr.decode())
    pass_one_audit=None
    if pass_one_command:
        resolved=subprocess.run(pass_one_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
        pass_one_report=json.loads(resolved.stdout)
        pass_one_audit=dict(command=pass_one_command,witness_binding=pass_one_witness,
            inventory=pass_one_checker.check_resolution(pass_one_report),report=pass_one_report,stderr=resolved.stderr.decode())
    collision_audit=None
    if collision_command:
        resolved=subprocess.run(collision_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
        collision_report=json.loads(resolved.stdout)
        collision_audit=dict(command=collision_command,witness_binding=collision_witness,
            inventory=collision_checker.check_resolution(collision_report),report=collision_report,stderr=resolved.stderr.decode())
    mapping_audit=None
    if mapping_command:
        resolved=subprocess.run(mapping_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
        mapping_report=json.loads(resolved.stdout)
        mapping_audit=dict(command=mapping_command,witness_binding=mapping_witness,
            inventory=mapping_checker.check_resolution(mapping_report,PE(BINARY/'build/link.exe')),report=mapping_report,stderr=resolved.stderr.decode())
    finder_audit=None
    if finder_command:
        resolved=subprocess.run(finder_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
        finder_report=json.loads(resolved.stdout)
        finder_audit=dict(command=finder_command,witness_binding=finder_witness,
            inventory=finder_checker.check_resolution(finder_report,PE(BINARY/'build/link.exe')),report=finder_report,stderr=resolved.stderr.decode())
    member_pointer_contract=None
    if 'LinkContext::Initialize' in trial['accepted']:
        from member_pointers import check_static
        from binary import PE
        member_pointer_contract=check_static(PE(ROOT/'toolchains/5.10/bin/LINK.EXE'),PE(BINARY/'build/link.exe'),(BINARY/'build/link.map').read_text(),trial)
    elapsed=time.perf_counter()-started
    after = {str(p):sha(p.read_bytes()) for p in inputs}
    assert before == after, 'audit inputs changed during discovery'
    report = json.loads(result.stdout)
    full_report = json.loads(full.stdout)
    import_report=json.loads(checked_imports.stdout)
    assert set(import_report['functions'])==set(imported['accepted'])
    assert report['discovery'] == 'cfg'
    assert full_report['discovery'] == 'cfg'
    assert set(report['functions']) == set(strict['accepted']), 'strict body subset differs'
    assert set(full_report['functions']) == set(trial['accepted']), 'full trial graph subset differs'
    return dict(elapsed_seconds=elapsed,total_elapsed_seconds=time.perf_counter()-audit_started,binding_and_setup_seconds=started-audit_started,delete_support=delete_support,register_import_audit=register_audit,full_link_import_audit=full_link_audit,pass_one_import_audit=pass_one_audit,collision_import_audit=collision_audit,mapping_import_audit=mapping_audit,finder_import_audit=finder_audit,member_pointer_contract=member_pointer_contract,full_command=full_command,strict_command=strict_command,import_command=import_command,import_report=import_report,cached_import_residual=residual,
                strict_projection_excludes_only_verified_import_thunks=sorted(import_leaves & set(trial['accepted'])),
                input_sha256_before=before, input_sha256_after=after,
                full_report=full_report,strict_report=report,
                qualification='Exact FF25 import leaves have no local RET; typed external ABI remains at imported-library boundary. Every actual application/CRT body retains strict RET comparison.',
                stderr=full.stderr.decode()+result.stderr.decode())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--write-trial', type=Path)
    parser.add_argument('--support',action='append',default=[],help='exact encountered import support leaf, separate from application credit')
    parser.add_argument('--original', type=Path)
    parser.add_argument('--native', type=Path)
    parser.add_argument('--native-cleanup',type=Path,help='fresh read-only final callback lifetime capture for normal completion')
    completion=parser.add_mutually_exclusive_group()
    completion.add_argument('--expected-next')
    completion.add_argument('--complete',action='store_true',help='require exact normal completion instead of a frontier')
    parser.add_argument('--promote', action='store_true')
    args = parser.parse_args()
    current = json.loads((BINARY / 'functions.json').read_text())
    trial = trial_for(current, args.candidate, args.support)
    if args.write_trial:
        args.write_trial.write_text(json.dumps(trial, indent=2) + '\n')
        return
    assert current.get('review_gate') == 'approved', 'root scaffold review has not approved acceptance'
    subprocess.run(['make','-q','native','original'], cwd=BINARY, check=True)
    assert args.original and args.native and (args.expected_next or args.complete), 'fresh captures and explicit frontier/completion required'
    bind_capture(args.original, trial, 'original')
    bind_capture(args.native, trial, 'native-complete'if args.complete else 'native')
    completion_master=None
    if args.complete:
        assert args.native_cleanup, 'normal completion requires fresh final cleanup state'
        bind_capture(args.native_cleanup,trial,'native')
        from completion_cleanup import verify as verify_cleanup
        cleanup=verify_cleanup(args.original,args.native_cleanup)
        completion_master=verify_completion(args.original,args.native,trial)
    else:verify_frontier(args.original, args.native, args.expected_next, trial)
    original_events = [json.loads(line) for line in (args.original / 'events.jsonl').read_text().splitlines()]
    assert args.candidate in [event['name'] for event in original_events if event['event']=='entry'], 'candidate not reached in original trial'
    for name in args.support:
        assert name in [event['name'] for event in original_events if event['event']=='entry'], 'support leaf not reached'
    native_state = json.loads((args.native / 'state.json').read_text())
    assert args.candidate in native_state.get('entries', []), 'candidate not reached in native trial'
    # Original binding above proves the exact executed mask/prefix/inputs.
    # Rebuilt aliases may be corrected without changing that execution; audit
    # the native capture's independently verified current trial index.
    audit = audit_trial(args.native / 'functions.json')
    validation = dict(candidate=args.candidate,case=json.loads((args.original/'receipt.json').read_text()).get('case','help'), expected_next=args.expected_next,completion=completion_master,
                      native_cleanup=str(args.native_cleanup)if args.complete else None,
                      cleanup=cleanup if args.complete else None,
                      trial_sha256=sha((args.native / 'functions.json').read_bytes()),
                      original_configuration_sha256=sha((args.original / 'functions.json').read_bytes()),
                      original=str(args.original), native=str(args.native), audit=audit)
    original_receipt=json.loads((args.original/'receipt.json').read_text())
    native_receipt=json.loads((args.native/'receipt.json').read_text())
    validation['evidence_bindings']=dict(
        original_receipt_sha256=sha((args.original/'receipt.json').read_bytes()),
        native_receipt_sha256=sha((args.native/'receipt.json').read_bytes()),
        original_native_at_capture_sha256=original_receipt['native_sha256'],
        tested_native_sha256=native_receipt['native_sha256'],
        native_only_retry=original_receipt['native_sha256']!=native_receipt['native_sha256'],
        original_execution='Exact target, mask, accepted prefix, argv, inputs and fixed working directory revalidated; native/shared rebuild does not change original execution.')
    (args.native / 'acceptance-validation.json').write_text(json.dumps(validation, indent=2) + '\n')
    if args.promote:
        if args.complete:trial['cases']['help']['complete']=True
        (BINARY / 'functions.json').write_text(json.dumps(trial, indent=2) + '\n')
    print(json.dumps(validation, indent=2))


if __name__ == '__main__': main()
