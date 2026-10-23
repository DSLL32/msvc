"""The reached member-function parameter record's exact two-load owner path.

This is a typed projection of the actual immediate caller's target_shape,
not general heap traversal or an additional callee ABI input.
"""
import re


def validate(function, formal, anchors, native, words, receipt, events, config, events_sha):
    assert function == '_equal_member_function_parameters@8'
    assert formal['name'] == 'target' and formal['stack_index'] == 1
    binding = formal['binding']
    assert binding['kind'] == 'member-function-parameter-owner'
    assert binding['offsets'] == [4, 8] and binding['owner_bytes'] == [12, 12]
    assert binding['original_register'] == 'ebx'
    assert binding['original_caller_rva'] == 0x54e9e
    owner_name = '_classify_member_pointer_conversion@12'
    assert binding['original_owner_function'] == owner_name and owner_name in config['accepted']
    registers = [r for r in receipt['ownership_register_observations'] if r['register'] == 'ebx']
    assert len(registers) == 1 and registers[0]['read_succeeded'], 'missing actual target-shape owner'
    original_root = registers[0]['value']
    assert re.findall(r'^ownership-anchor ebx ([0-9a-f]{8})$', events, re.M) == ['%08x' % original_root]
    assert re.findall(r'^caller ([0-9a-f]{8})$', events, re.M) == ['00054e9e']
    projection = receipt['ownership_entry_projection']
    assert owner_name in projection['observed'] and projection['events_sha256'] == events_sha
    owner_record = next(r for r in config['driver'] if r['name'] == owner_name)
    assert re.search(r'^entry '+re.escape(owner_name)+r' rva %08x byte [0-9a-f]{2}$' % (owner_record['start']-config['target']['base']), events, re.M)
    owner = native['caller_locals'][binding['native_owner_field']]
    assert owner['function'] == 'classify_member_pointer_conversion' and owner['local'] == 'target_shape'
    assert owner['fields'] == [] and owner['member_parameter_path'] is True
    native_root = owner['root']
    original_previous, native_previous = original_root, native_root
    assert len(owner['pointer_path']) == 2, 'incomplete native parameter owner'
    for index, (name, offset) in enumerate(zip(('target_signature', 'target_parameters'), (4, 8))):
        original = anchors[name]
        actual = owner['pointer_path'][index]
        assert 0 < original_previous <= 0xffffffff-12 and 0 < native_previous <= 0xffffffff-12, 'NULL/wrapping typed owner'
        assert original['bytes'] == original['read_bytes'] == 4 and original['read_succeeded'], 'missing exact owner pointer read'
        assert original['original'] == original_previous+offset, 'original parameter owner link differs'
        assert actual['address'] == native_previous+offset and actual['bytes'] == actual['read_bytes'] == 4 and actual['read_succeeded'], 'native parameter owner link differs'
        assert original['value'] and actual['value'], 'NULL/incomplete parameter owner'
        original_previous, native_previous = original['value'], actual['value']
    assert words[1] == formal['original_raw'] == original_previous, 'actual target argument differs from original owner'
    assert formal['native_raw'] == owner['value'] == native_previous, 'actual target argument differs from native owner'
