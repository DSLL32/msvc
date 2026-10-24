"""One complete compact data-member conversion never uses its path formal.

Retain both raw ABI words without asserting pointer identity or pointee state.
The exact body and four table aliases restrict this to the reviewed callback.
"""


def validate(function, formal, native, words, config):
    assert function == '_convert_compact_data_member_pointer@24'
    assert formal['name'] == 'path' and formal['stack_index'] == 2
    binding = formal['binding']
    assert binding['kind'] == 'unused-compact-data-path'
    assert binding['body_extent'] == [0x4bee0, 0x4bf5f]
    assert binding['consumed_stack_indices'] == [0, 1, 4]
    assert binding['table'] == '_member_pointer_conversions'
    assert binding['callback_slots'] == [16, 17, 20, 21]
    tables = [row for row in config['tables'] if row.get('name') == binding['table']]
    assert len(tables) == 1 and tables[0]['kind'] == 'function_pointers'
    assert len(tables[0]['functions']) == 32
    assert [index for index, name in enumerate(tables[0]['functions']) if name == function] == binding['callback_slots'], 'compact callback table aliases differ'
    assert formal['original_raw'] == words[2]
    assert formal['native_raw'] == native[formal['native_field']]
    values = []
    for local in ('source_model', 'target_model', 'data_member'):
        owner = native['caller_locals']['caller_' + local]
        assert owner['function'] == 'convert_member_pointer_representation'
        assert owner['local'] == local and owner.get('fields', []) == []
        values.append(owner['value'])
    source, target, data = values
    assert source in (1, 2) and target in (1, 2) and data == 1, 'actual callback selection differs'
    assert 16 + 4 * (source - 1) + target - 1 in binding['callback_slots']
    return 'uncredited-unused'
