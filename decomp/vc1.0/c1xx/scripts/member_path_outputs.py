"""Exact address-only caller output locals for the reached member-path query."""


def validate(function, formal, owner):
    assert function == '_resolve_member_pointer_path@12'
    expected = {'virtual_base': (1, 32), 'displacement': (2, 28)}
    index, offset = expected[formal['name']]
    binding = formal['binding']
    assert formal['stack_index'] == index and binding['original_stack_offset'] == offset
    assert binding['original_register'] == 'esp' and binding['native_owner_address']
    assert binding['original_caller_rva'] == 0x3a073 and binding['original_owner_function'] == '_member_pointer_expression@4'
    assert owner['function'] == 'member_pointer_expression' and owner['local'] == formal['name']
    assert owner['value'] is None and binding['output_only'], 'ignored output prestate must remain uncredited'
    assert 'original_prestate_word_anchor' not in binding
