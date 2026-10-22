#!/usr/bin/env python3
"""Check initial bytes and address aliases of encountered frontend state."""
from compare import indexes, publics

original, rebuilt, _, _, _ = indexes()
names = publics()
# Original locations identify semantic objects, never reconstruction storage.
STATE = [
    ('_current_function', 0x5b228, 4),
    ('_global_expression_scope_active', 0x5b25c, 4),
    ('_global_record_kind', 0x6003c, 1),
    ('_function_ir', 0x60044, 20),
    ('_function_in_pch', 0x6bf64, 4),
    ('_pch_symbol_offset_adjustment', 0x6b804, 4),
    ('_function_call_seen', 0x6bfb0, 4),
    ('_default_inline_candidate', 0x6bfa8, 4),
    ('_inline_candidate', 0x6bfac, 4),
    ('_function_not_inlinable', 0x6bfb4, 4),
    ('_function_records', 0x6b63c, 4),
    ('_last_function_records', 0x6b640, 4),
    ('_function_record_index', 0x6b7e4, 4),
    ('_conditional_depth', 0x6bf74, 4),
    ('_include_depth', 0x6a7d4, 2),
    ('_source_handle', 0x6a7d8, 4),
    ('_pch_include_state', 0x6be70, 1),
    ('_pch_expression_output', 0x6be80, 4),
    ('_pch_file_incomplete', 0x6de30, 4),
    ('_pch_replacement_incomplete', 0x6de34, 4),
    ('_pch_file', 0x5b580, 4),
    ('_pch_replacement_file', 0x5b264, 4),
    ('_pending_global_symbols', 0x6b6bc, 4),
    ('_parser_lookahead', 0x6b550, 4),
    ('_aggregate_depth', 0x6bf94, 4),
    ('_scope_depth', 0x6ce6c, 1),
    ('_permanent_page_count', 0x6de00, 4),
    ('_pch_record_cursor', 0x5b568, 4),
    ('_pch_include_count', 0x6de1c, 2),
    ('_pch_include_page_count', 0x6de20, 2),
    ('_pch_macro_changed', 0x6de24, 4),
    ('_pch_rebuild_on_macro_change', 0x6de28, 4),
    ('_conditional_branch_state', 0x638a0, 128),
    ('_macro_parameter_count', 0x63940, 4),
    ('_preprocessor_expression_token', 0x6a7b0, 1),
    ('_preprocessor_parentheses', 0x6a7b4, 4),
    ('_pp_unevaluated_depth', 0x64d78, 4),
    ('_lexer_text_storage', 0x5b158, 132),
    ('_lexer_text_storage', 0x5b1d8, 4, 128),
]

def read(image, address, size):
    section = next(s for s in image.sections if
                   s['rva'] <= address-image.base < s['rva']+s['virtual_size'])
    return bytes(size) if section['name'] == '.bss' else image.read(address, size)

forward, backward = {}, {}
for field in STATE:
    name, old, size = field[:3]
    new = names[name] + (field[3] if len(field) > 3 else 0)
    assert read(original, old, size) == read(rebuilt, new, size), 'state default differs: '+name
    for offset in range(size):
        a, b = old+offset, new+offset
        assert forward.setdefault(a, b) == b, 'shared state was split: '+name
        assert backward.setdefault(b, a) == a, 'distinct state was merged: '+name
print('%d encountered frontend state defaults and aliases match.' % len(STATE))
