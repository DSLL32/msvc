"""Explicit C1XX entry-argument observations, outside native fixtures.

Plans contain only reviewed stack words and scalar/static-object identities.
The original observer defaults to four words, with an explicit six-word option.
Unsupported transport holds rather than guessing registers or dumping objects.
"""
from member_parameter_owner import validate as validate_member_parameter_owner
from member_path_outputs import validate as validate_member_path_output
from goto_cleanup_inputs import validate as validate_goto_cleanup_inputs
import argparse
from parser_context_owner import validate as validate_parser_context_owner
from virtual_table_owner import validate as validate_virtual_table_owner, validate_iterator as validate_virtual_iterator
from unused_conversion_text import validate as validate_unused_conversion_text
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from argument_words import observation as argument_observation

BINARY = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True)
    parser.add_argument('--plan', type=Path, required=True)
    args = parser.parse_args()
    started = time.perf_counter()
    plan = json.loads(args.plan.read_text())
    config = json.loads((BINARY/'functions.json').read_text())
    name = config['cases'][args.case]['terminal']
    assert plan['function'] == name
    assert not plan['hidden_inputs'] and not plan['unknown_inputs'], 'unsupported ABI transport holds acceptance'
    assert not plan['register_inputs'] or ((name=='_check_extended_range' and plan.get('x87_inputs')) or (name in ('_store_extended','_finish_extended_arithmetic') and plan.get('x87_single'))), 'unsupported register ABI transport holds acceptance'
    for formal in plan['formals']:
        if formal['binding']['kind']=='parser-context-slot':
            from parser_context_owner import STACK_BASE, STACK_ELEMENTS
            slot=next(row for row in plan['original_observations'] if row['name']=='parser_context')
            assert STACK_BASE<=slot['original']<STACK_BASE+(STACK_ELEMENTS-1)*8 and (slot['original']-STACK_BASE)%8==0, 'parser observation outside allocated semantic stack'
    requested_words = plan.get('argument_words', 4)
    assert requested_words in (4, 6)
    assert all(0 <= item['stack_index'] < requested_words for item in plan['formals']), 'formal exceeds requested original stack words'
    command = [sys.executable, str(BINARY/'scripts/capture-original.py'), '--case', args.case, '--argument-words', str(requested_words)]
    if plan.get('x87_inputs'):command += ['--x87-inputs']
    if plan.get('x87_single'):command += ['--x87-single']
    for item in plan.get('original_observations', []):
        assert item.get('bytes',4) in (2,4)
        command += ['--observe-short' if item.get('bytes',4)==2 else '--observe-state', item['name']+'='+hex(item['original'])]
    if plan.get('ownership_register'):
        owners = {item['binding']['original_owner_function'] for item in plan['formals'] if item['binding']['kind'] in ('caller-owned-pointer','parser-context-slot','member-function-parameter-owner')}
        if plan.get('x87_inputs') or plan.get('x87_single'):owners.add(plan['x87_owner']['original_function'])
        assert len(owners)==1, 'one explicit actual ownership caller per observation'
        command += ['--ownership-register', plan['ownership_register'], '--entry-plan', '--entry-owner', owners.pop()]
    original = Path(subprocess.check_output(command, cwd=BINARY, text=True).strip().splitlines()[-1]).parent
    out = BINARY/'cases/argument-observations'/args.case/('%04d-' % len(config['accepted']) + re.sub(r'[^A-Za-z_0-9-]', '-', name.lstrip('_@')))
    out.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location('frontier_argument_native', BINARY/'scripts/capture-native.py')
    native = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(native)
    checkpoint = native.capture(out/'native-raw', breakpoint=plan['native_entry'], expressions=plan['native_expressions'], case_name=args.case, caller_locals=plan.get('native_caller_locals'), x87_inputs='single' if plan.get('x87_single') else plan.get('x87_inputs',False))
    receipt = json.loads((original/'receipt.json').read_text())
    events = (original/'events.txt').read_text()
    assert receipt['terminal'] == name and receipt['configuration_sha256'] == sha(BINARY/'functions.json')
    assert checkpoint['function'] == plan['native_entry']
    assert checkpoint['declared_arguments'] == sorted(item['name'] for item in plan['formals'])
    words = {int(i): int(v, 16) for i, v in re.findall(r'^argument (\d+) ([0-9a-f]+)$', events, re.M)}
    observed_words = argument_observation(events, requested_words)
    assert observed_words == receipt['argument_observation']
    anchors = {item['name']: item for item in receipt.get('diagnostic_observations', [])}
    states = [int(s, 16) for s in re.findall(r'^state \d+ ([0-9a-f]+)$', events, re.M)]
    for requirement in plan.get('state_requirements', []):
        assert states[requirement['index']] == checkpoint[requirement['native_field']] == requirement['equals']
    x87_result=None
    if plan.get('x87_inputs') or plan.get('x87_single'):
        from x87_inputs import validate_range_inputs,validate_store_inputs,validate_finish_inputs
        validator={'_check_extended_range':validate_range_inputs,'_store_extended':validate_store_inputs,'_finish_extended_arithmetic':validate_finish_inputs}[name]
        x87_result=validator(name,plan,receipt,events,checkpoint,anchors,config)
    for item in plan['formals']:
        item['original_raw'] = words[item['stack_index']]
        item['native_raw'] = checkpoint[item['native_field']]
        binding = item['binding']
        if binding['kind'] == 'scalar':
            mask = (1 << binding['bits']) - 1
            assert item['original_raw'] & mask == item['native_raw'] & mask
        elif binding['kind'] == 'extended-stack-value':
            from extended_arguments import validate as validate_extended_argument
            item['by_value_words']=validate_extended_argument(name,item,checkpoint,words)
        elif binding['kind'] == 'unused-conversion-text':
            validate_unused_conversion_text(name, item, anchors, checkpoint, words)
        elif binding['kind'] == 'class-first-virtual-table':
            validate_virtual_table_owner(name, item, anchors, checkpoint, words)
        elif binding['kind'] == 'parser-context-slot':
            validate_parser_context_owner(name, item, anchors, checkpoint, words, receipt, events, config, sha(original/'events.txt'))
        elif binding['kind'] == 'member-function-parameter-owner':
            validate_member_parameter_owner(name, item, anchors, checkpoint, words, receipt, events, config, sha(original/'events.txt'))
        elif binding['kind'] == 'null-pointer':
            assert item['original_raw'] == item['native_raw'] == 0
        elif binding['kind'] == 'object-offset':
            binding['native_object_address'] = checkpoint[binding['native_anchor_field']]
            assert item['original_raw'] - binding['original_object'] == binding['offset']
            assert item['native_raw'] - binding['native_object_address'] == binding['offset']
        elif binding['kind'] == 'pointer-alias':
            anchor = anchors[binding['original_anchor_name']]
            assert anchor['original'] == binding['original_anchor_address']
            binding['original_anchor_value'] = anchor['value']
            binding['native_anchor_value'] = checkpoint[binding['native_anchor_field']]
            offset = binding.get('offset', 0)
            if 'offset' in binding:
                assert anchor['bytes']==anchor['read_bytes']==4 and anchor['read_succeeded'], 'missing readable global buffer owner'
                assert binding['owner_bytes']==4096 and 0<=offset<binding['owner_bytes'], 'debug buffer offset outside owner span'
                assert 0<binding['original_anchor_value']<=0xffffffff-binding['owner_bytes'], 'original buffer owner span wraps'
                assert 0<binding['native_anchor_value']<=0xffffffff-binding['owner_bytes'], 'native buffer owner span wraps'
            assert item['original_raw'] == binding['original_anchor_value'] + offset, 'actual original pointer differs from fresh global owner plus offset'
            assert item['native_raw'] == binding['native_anchor_value'] + offset, 'actual native pointer differs from fresh global owner plus offset'
        elif binding['kind'] == 'caller-owned-pointer':
            registers = [row for row in receipt['ownership_register_observations'] if row['register'] == binding['original_register']]
            assert len(registers) == 1 and registers[0]['read_succeeded']
            assert re.findall(r'^caller ([0-9a-f]+)$', events, re.M) == ['%08x' % binding['original_caller_rva']]
            projection=receipt.get('ownership_entry_projection', receipt.get('first_entry_projection'))
            assert binding['original_owner_function'] in projection['observed']
            assert binding['original_owner_function'] in config['accepted']
            assert projection['events_sha256'] == sha(original/'events.txt')
            owner_record=next(row for row in config['driver'] if row['name']==binding['original_owner_function'])
            assert re.search(r'^entry '+re.escape(binding['original_owner_function'])+r' rva %08x byte [0-9a-f]{2}$' % (owner_record['start']-config['target']['base']), events, re.M), 'actual accepted owner entry/RVA missing'
            owner = checkpoint['caller_locals'][binding['native_owner_field']]
            assert owner['function'] == binding['native_owner_function'] and owner['local'] == binding['native_owner_local']
            assert owner.get('fields', []) == binding.get('native_owner_fields', []), 'actual caller field differs'
            offset = binding.get('original_stack_offset', 0)
            binding['original_owner_value'] = registers[0]['value'] + offset
            binding['native_owner_value'] = owner['address'] if binding.get('native_owner_address') else owner['value']
            assert item['original_raw'] == binding['original_owner_value'], 'original caller owner plus offset differs'
            assert item['native_raw'] == binding['native_owner_value'], 'native actual caller local owner differs'
            if 'original_stack_offset' in binding and name=='_resolve_member_pointer_path@12':
                validate_member_path_output(name, item, owner)
            elif 'original_stack_offset' in binding and name=='_multiply_extended':
                from extended_arithmetic_owners import validate as validate_extended_arithmetic_owner
                validate_extended_arithmetic_owner(name, item, owner)
            elif 'original_stack_offset' in binding and name=='_fold_floating_binary@12':
                from floating_binary_owners import validate as validate_floating_binary_owner
                validate_floating_binary_owner(name, item, owner)
            elif 'original_stack_offset' in binding and name=='_double_to_extended':
                assert offset==28 and binding['original_register']=='esp' and binding['native_owner_address'], 'wrong rounded double input stack owner'
                assert binding['original_caller_rva']==0x64c43 and binding['original_owner_function']=='_convert_floating_literal@8'
                assert owner['function']=='convert_floating_literal' and owner['local']=='real', 'wrong rounded double input local'
                assert owner['value'] is None and binding['address_only'] and not binding.get('output_only'), 'only actual initialized input owner address is observed'
                assert 'original_prestate_word_anchor' not in binding
            elif 'original_stack_offset' in binding and name=='_extended_to_double':
                assert offset==28 and binding['original_register']=='esp' and binding['native_owner_address'], 'wrong double output stack owner'
                assert binding['original_caller_rva']==0x64c21 and binding['original_owner_function']=='_convert_floating_literal@8'
                assert owner['function']=='convert_floating_literal' and owner['local']=='real', 'wrong double output local owner'
                assert owner['value'] is None and binding['output_only'], 'double output prestate is uncredited'
                assert 'original_prestate_word_anchor' not in binding
            elif 'original_stack_offset' in binding:
                assert offset in (34,36) and binding['original_register']=='esp' and binding['native_owner_address'], 'unsupported caller stack owner offset'
                if offset==34:
                    assert owner['function']=='emit_debug_field_list' and owner['local']=='conversion_text', 'actual output local owner differs'
                    assert owner['value'] is None and binding['output_only'], 'unwritten output local must be address-only'
                    assert 'original_prestate_word_anchor' not in binding, 'unwritten output prestate is not observed'
                else:
                    assert owner['function']=='emit_debug_field_list' and owner['local']=='member_index'
                    word=anchors[binding['original_prestate_word_anchor']]
                    assert word['bytes']==word['read_bytes']==4 and word['read_succeeded'], 'missing readable caller counter prestate'
                    assert word['original']==item['original_raw'], 'caller counter observation address differs'
                    assert word['value']==owner['value']==checkpoint[binding['native_pointee_field']]==0, 'defined caller counter prestate differs'
            if binding.get('virtual_iterator_prestate'):
                validate_virtual_iterator(name, item, anchors, checkpoint, words)
        elif binding['kind'] == 'pointee-word':
            anchor = anchors[binding['original_observation_name']]
            assert item['original_raw'] + binding.get('word_offset', 0) == anchor['original'] == binding['original_pointee_address'], 'actual argument no longer names observed word'
            binding['original_word'] = anchor['value']
            binding['native_word'] = checkpoint[binding['native_pointee_field']]
            assert binding['original_word'] == binding['native_word']
            if binding.get('native_semantic_owner_field'):
                assert binding['native_word'] == checkpoint[binding['native_semantic_owner_field']]
        elif binding['kind'] == 'debug-queue-word':
            queue=anchors[binding['original_queue_anchor']]
            word=anchors[binding['original_word_anchor']]
            assert word['bytes']==word['read_bytes']==2 and word['read_succeeded'], 'exact initialized WORD observation required'
            owner=checkpoint['caller_locals'][binding['native_record_field']]
            assert owner['function']=='debug_tag_type' and owner['local']=='record' and owner['value']==binding['slot']
            offset=16*binding['slot']+12
            assert item['original_raw']==queue['value']+offset==word['original']
            assert item['native_raw']==checkpoint[binding['native_queue_field']]+offset
            binding['original_word']=word['value']
            binding['native_word']=checkpoint[binding['native_word_field']]
            assert binding['original_word']==binding['native_word']
        else:
            raise AssertionError('unsupported logical argument binding: '+item['name'])
        item['result'] = 'uncredited-unused' if binding['kind']=='unused-conversion-text' else 'match'
    if name=='_walk_goto_cleanup@16':
        validate_goto_cleanup_inputs(name, plan['formals'], anchors, checkpoint)
    assert states[:len(config['checkpoint_state'])] == checkpoint['states']
    assert len(states) == len(config['checkpoint_state']) + len(anchors)
    assert [m[1] for m in re.findall(r'^buffer \d+ (\d+) ([0-9a-f]*)$', events, re.M)] == checkpoint['buffers']
    for filename in ('receipt.json', 'events.txt', 'configuration.json', 'stdout.bin', 'stderr.bin'):
        shutil.copyfile(original/filename, out/('original-'+filename))
    if (original/'entries.plan').exists():
        shutil.copyfile(original/'entries.plan', out/'original-entries.plan')
    for filename in ('checkpoint.json', 'commands.gdb', 'gdb.log', 'timing.json', 'stdout.bin', 'stderr.bin'):
        shutil.copyfile(out/'native-raw'/filename, out/('native-'+filename))
    shutil.rmtree(out/'native-raw')
    start, end = plan['abi_analysis']['original_extent']
    (out/'original-disassembly.txt').write_bytes(subprocess.check_output(['objdump', '-d', '-Mintel', '--start-address='+hex(start), '--stop-address='+hex(end), '../../../toolchains/1.0-nt/bin/C1XX3232.EXE'], cwd=BINARY))
    if plan.get('ownership_caller_extent'):
        start,end = plan['ownership_caller_extent']
        (out/'original-ownership-caller-disassembly.txt').write_bytes(subprocess.check_output(['objdump', '-d', '-Mintel', '--start-address='+hex(start), '--stop-address='+hex(end), '../../../toolchains/1.0-nt/bin/C1XX3232.EXE'], cwd=BINARY))
    shutil.copyfile(args.plan, out/'plan.json')
    plan['abi_analysis']['original_target_sha256'] = receipt['original_sha256']
    proof = dict(schema=1, status='match', function=name, case=args.case, accepted_before=len(config['accepted']), configuration_sha256=sha(BINARY/'functions.json'), native_entry=plan['native_entry'], declared_formal_count=len(plan['formals']), formals=plan['formals'], register_inputs=x87_result['registers'] if x87_result else [], hidden_inputs=[], unknown_inputs=[], abi_analysis=plan['abi_analysis'], native_executable_sha256=sha(BINARY/'build/native/C1XX3232.EXE'), capture_timing=dict(original_capture_seconds=receipt['capture_seconds'], native_capture_seconds=json.loads((out/'native-timing.json').read_text())['total_seconds'], total_operation_seconds=time.perf_counter()-started), evidence_sha256={p.name:sha(p) for p in sorted(out.iterdir()) if p.is_file()})
    if x87_result:proof.update(x87_inputs=bool(plan.get('x87_inputs')),x87_single=bool(plan.get('x87_single')),x87_owner=plan['x87_owner'],x87_environment=x87_result['environment'])
    proof['uncredited_unused_inputs'] = [item['name'] for item in plan['formals'] if item['result']=='uncredited-unused']
    proof['argument_observation'] = observed_words
    proof['state_requirements'] = plan.get('state_requirements', [])
    (out/'validation.json').write_text(json.dumps(proof, indent=2)+'\n')
    print(out/'validation.json')


if __name__ == '__main__':
    main()
