"""The actual component-zero caller slot is output-only until its first write."""


def validate(function, formals, native, words):
    assert function == '_member_pointer_component@12'
    assert words[1] == native['raw_component'] == 0, 'output-only proof requires actual component zero'
    formal = next(row for row in formals if row['name'] == 'saved')
    assert formal['stack_index'] == 2
    binding = formal['binding']
    assert binding['kind'] == 'caller-owned-pointer' and binding['original_register'] == 'eax'
    assert binding['original_caller_rva'] == 0x3bf27
    assert binding['original_owner_function'] == '_convert_compact_data_member_pointer@24'
    assert binding['native_owner_address'] and binding['address_only'] and binding['output_only']
    owner = native['caller_locals'][binding['native_owner_field']]
    assert owner['function'] == 'convert_compact_data_member_pointer' and owner['local'] == 'saved'
    assert owner.get('fields', []) == [] and owner['value'] is None, 'unwritten saved prestate must stay unobserved'
    assert owner['address'] == formal['native_raw'] == native[formal['native_field']]
    assert words[2] == formal['original_raw']
