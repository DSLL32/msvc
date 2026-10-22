#!/usr/bin/env python3
"""Check initial bytes and address aliases of encountered frontend state."""
from compare import indexes, publics
import struct
import re
from pathlib import Path

original, rebuilt, config, _, _ = indexes()
names = publics()
# Original locations identify semantic objects, never reconstruction storage.
STATE = [
    ('_error_count', 0x5b228, 4),
    ('_global_expression_scope_active', 0x5b25c, 4),
    ('_global_record_kind', 0x6003c, 1),
    ('_string_record_count', 0x60040, 4),
    ('_auxiliary_ir_stream', 0x6e434, 4),
    ('_initializer_tag', 0x60038, 1),
    ('_initializer_buffer', 0x5f4f4, 4),
    ('_pending_initializer_symbol', 0x6c068, 4),
    ('_initializer_symbol', 0x6023c, 4),
    ('_initializer_frame_storage', 0x6025c, 512, 16),
    ('_initializer_symbol_copy_sizes', 0x6c070, 80),
    ('_initializer_frame_storage', 0x60258, 4, 12),
    ('_initializer_depth', 0x6cd14, 4),
    ('_initializer_frame_storage', 0x60254, 4, 8),
    ('_initializer_inferred_type', 0x60230, 4),
    ('_initializer_offset', 0x60244, 4),
    ('_initializer_bitfield_offset', 0x60248, 4),
    ('_initializer_frame_storage', 0x6024c, 4),
    ('_initializer_frame_storage', 0x60250, 4, 4),
    ('_initializer_runtime_override', 0x6045c, 4),
    ('_initializer_reset_words', 0x6cd18, 12),
    ('_symbol_name_buffer', 0x65e60, 248),
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
    ('_browser_filename', 0x5b588, 4),
    ('_final_newline_emitted', 0x6b140, 4),
    ('_pch_include_state', 0x6be70, 1),
    ('_pch_expression_output', 0x6be80, 4),
    ('_pch_file_incomplete', 0x6de30, 4),
    ('_pch_replacement_incomplete', 0x6de34, 4),
    ('_pch_file', 0x5b580, 4),
    ('_pch_replacement_file', 0x5b264, 4),
    ('_pending_global_symbols', 0x6b6bc, 4),
    ('_declaration_controls', 0x60798, 64),
    ('_declaration_controls', 0x6079b, 1, 3),
    ('_declaration_controls', 0x6079c, 60, 4),
    ('_parser_lookahead', 0x6b550, 4),
    ('_parser_flags', 0x6bb48, 4),
    ('_parameter_nesting', 0x5b13c, 1),
    ('_current_parameter', 0x60064, 4),
    ('_current_label_symbol', 0x60068, 4),
    ('_label_stack_count', 0x60088, 4),
    ('_continue_labels', 0x6008c, 4),
    ('_break_labels', 0x60090, 4),
    ('_do_labels', 0x60094, 4),
    ('_default_labels', 0x60098, 4),
    ('_switch_labels', 0x6009c, 4),
    ('_case_label', 0x600a0, 4),
    ('_pointer_base_symbol', 0x5ffbc, 4),
    ('_inline_base_region', 0x5ffd0, 2),
    ('_inline_regions', 0x5ffd4, 4),
    ('_inline_scope_depth', 0x5ffd8, 4),
    ('_inline_scopes', 0x5ffdc, 4),
    ('_inline_region_index', 0x5ffe0, 2),
    ('_inline_parameter_slots', 0x5ffe4, 80),
    ('_inline_slot_count', 0x60034, 1),
    ('_case_magnitude_limit', 0x6bfd8, 20),
    ('_scope_serial', 0x6bff0, 4),
    ('_function_scope_level', 0x6bfec, 2),
    ('_call_context', 0x6b5d4, 4),
    ('_call_designator_depth', 0x6bf6c, 4),
    ('_code_pointer_bytes', 0x5f4ec, 4),
    ('_type_0000', 0x6b700, 4),
    ('_exception_scope_depth', 0x6006c, 12),
    ('_name_unnamed', 0x6b680, 4),
    ('_name_setjmp', 0x6b6a8, 4),
    ('_name_catch', 0x6b6ac, 4),
    ('_next_symbol_index', 0x6b7f4, 4),
    ('_browser_symbol_limit', 0x6b7f8, 4),
    ('_current_token_value', 0x5b150, 8),
    ('_target_signed_int_maximum', 0x6bfd0, 4),
    ('_extended_zero', 0x6c0f0, 12),
    ('_constant_one', 0x5ffc8, 4),
    ('_expression_storage', 0x5f508, 2052),
    ('_text_work_cursor', 0x6bff8, 4),
    ('_scope_cleanup_stack', 0x60078, 4),
    ('_aggregate_contexts', 0x600a8, 384),
    ('_tag_in_current_scope', 0x604a4, 4),
    ('_enumerator_value', 0x6bfb8, 4),
    ('_enumerator_context', 0x6b648, 4),
    ('_current_enum_tag', 0x60830, 4),
    ('_pch_separate_debug', 0x6be84, 4),
    ('_aggregate_depth', 0x6bf94, 4),
    ('_global_scope', 0x6b730, 4),
    ('_current_scope', 0x6b734, 4),
    ('_lookup_scope', 0x6b65c, 4),
    ('_scope_directory', 0x6b738, 4),
    ('_found_symbol_scope', 0x6007c, 4),
    ('_found_scope_depth', 0x5b138, 1),
    ('_scope_depth', 0x6ce6c, 1),
    ('_permanent_page_count', 0x6de00, 4),
    ('_pch_record_cursor', 0x5b568, 4),
    ('_pch_include_count', 0x6de1c, 2),
    ('_pch_include_page_count', 0x6de20, 2),
    ('_pch_macro_changed', 0x6de24, 4),
    ('_pch_rebuild_on_macro_change', 0x6de28, 4),
    ('_conditional_branch_state', 0x638a0, 128),
    ('_macro_work_bytes', 0x6bff4, 4),
    ('_macro_work_buffer', 0x6078c, 4),
    ('_macro_replacement_text', 0x6393c, 4),
    ('_macro_expansion_next', 0x63898, 4),
    ('_macro_replacement_start', 0x63934, 4),
    ('_macro_expansion_start', 0x63938, 4),
    ('_macro_parameter_count', 0x63940, 4),
    ('_macro_space_seen', 0x63928, 4),
    ('_browser_macro_context', 0x63868, 4),
    ('_macro_frames', 0x63944, 5120),
    ('_macro_frames', 0x63958, 4, 20),
    ('_macro_expansion_active', 0x63854, 4),
    ('_macro_rescan_depth', 0x6389c, 4),
    ('_macro_definition_line', 0x6392c, 4),
    ('_macro_input_mark', 0x63930, 4),
    ('_macro_actual_argument_count', 0x64d44, 4),
    ('_preprocessor_expression_token', 0x6a7b0, 1),
    ('_preprocessor_parentheses', 0x6a7b4, 4),
    ('_pp_unevaluated_depth', 0x64d78, 4),
    ('_preprocessor_literal', 0x64d7c, 8),
    ('_lexer_text_storage', 0x5b158, 132),
    ('_lexer_text_storage', 0x5b1d8, 4, 128),
    ('_normalized_Gs', 0x6bf8c, 4),
    ('_normalized_Gn', 0x6bf90, 4),
    ('_normalized_PLMN', 0x5b208, 4),
    ('_normalized_PLMF', 0x5b204, 4),
    ('_normalized_Zr', 0x5b20c, 4),
    ('_normalized_Oq', 0x5b1fc, 4),
    ('_normalized_Ol', 0x5b200, 4),
    ('_normalized_Oa', 0x6c034, 4),
    ('_normalized_Ow', 0x6c038, 4),
    ('_normalized_Op', 0x6c048, 4),
    ('_normalized_Oz', 0x6c04c, 4),
    ('_normalized_Oy', 0x6c060, 4),
    ('_warning_levels', 0x6b818, 800),
    ('_lowest_warning_700', 0x6bb38, 4),
    ('_highest_warning_700', 0x6bb3c, 4),
    ('_pragma_argument', 0x63920, 4),
    ('_listing_number', 0x63850, 2),
    ('_listing_subtitle', 0x604b4, 4),
    ('_listing_title', 0x604b0, 4),
    ('_optimization_pragma_text', 0x604ac, 4),
    ('_optimization_pragma_on', 0x60834, 4),
    ('_native_caller_pragma_seen', 0x6bbcc, 4),
    ('_default_string_pooling', 0x6bc08, 4),
    ('_default_code_segment', 0x6b7b0, 4),
    ('_default_data_segment', 0x6b7a8, 4),
    ('_default_const_segment', 0x6b7a4, 4),
    ('_default_bss_segment', 0x6b7a0, 4),
    ('_plmn_pragma_seen', 0x6bbc8, 4),
    ('_plmf_pragma_seen', 0x6bbc4, 4),
    ('_active_plmn_pragma', 0x6b7f0, 4),
    ('_active_plmf_pragma', 0x6b7ec, 4),
    ('_pragma_optimization_restore_mask', 0x6ef3c, 4),
    ('_pragma_optimization_disable_mask', 0x6ef40, 4),
]

def read(image, address, size):
    section = next(s for s in image.sections if
                   s['rva'] <= address-image.base < s['rva']+s['virtual_size'])
    return bytes(size) if section['name'] == '.bss' else image.read(address, size)

forward, backward = {}, {}
def check(name, old, new, size):
    if name == '_text_work_cursor':
        # Initialized pointers must retain semantic targets after relocation.
        assert struct.unpack('<I', read(original, old, 4))[0] == 0x5f508
        assert struct.unpack('<I', read(rebuilt, new, 4))[0] == names['_expression_storage']
    else:
        assert read(original, old, size) == read(rebuilt, new, size), 'state default differs: '+name
    for offset in range(size):
        a, b = old+offset, new+offset
        assert forward.setdefault(a, b) == b, 'shared state was split: '+name
        assert backward.setdefault(b, a) == a, 'distinct state was merged: '+name

for field in STATE:
    name, old, size = field[:3]
    new = names[name] + (field[3] if len(field) > 3 else 0)
    check(name, old, new, size)

# CompilerOptions groups originally separate four-byte option objects. Include
# its scalar defaults and byte aliases in the same address map as parser state.
option_fields = re.findall(r'^    (int|[^;]+?\*)\s*(\w+); /\* original (0x[0-9a-f]+) \*/$',
                           Path('options.h').read_text(), re.M)
assert len(option_fields) == 94, len(option_fields)
option_scalar_count = 0
for index, (kind, field, address) in enumerate(option_fields):
    if kind == 'int':
        check('options.' + field, int(address, 16), names['_options'] + 4*index, 4)
        option_scalar_count += 1

# Pointer tables must refer to the same semantic objects as their direct users.
old_options = struct.unpack('<21I', read(original, 0x6ef48, 84))
new_options = struct.unpack('<21I', read(rebuilt, names['_packed_option_values'], 84))
for index, (old, new) in enumerate(zip(old_options, new_options)):
    check('packed option %d' % index, old, new, 4)
# Pragma optimization rules must update those very same option objects.
for index in range(18):
    old = read(original, 0x6b150 + 8 * index, 8)
    new = read(rebuilt, names['_pragma_optimization_rules'] + 8 * index, 8)
    assert old[:2] == new[:2], ('pragma optimization rule', index)
    if old[0]:
        old_target, new_target = struct.unpack_from('<I', old, 4)[0], struct.unpack_from('<I', new, 4)[0]
        assert forward[old_target] == new_target, ('pragma option target', index)

# Generic hash dispatch keeps all original callback identities, including traps.
callbacks = [('_hash_functions', 0x6ce60,
              ['_hash_interned_name', '_hash_pointer_low12', '_hash_symbol_index']),
             ('_hash_equal_functions', 0x6ce58,
              ['_hash_keys_equal', '_hash_symbol_indices_equal'])]
entries = {entry['name']: entry['start'] for entry in config['driver']}
for table, address, functions in callbacks:
    for index, name in enumerate(functions):
        assert struct.unpack('<I', read(original, address + 4 * index, 4))[0] == entries[name], name
        assert struct.unpack('<I', read(rebuilt, names[table] + 4 * index, 4))[0] == names[name], name
# Intrinsic lookup uses suffixes of the same vocabulary as registration.
# Check every logical suffix/group boundary against that second original view.
assert read(original, 0x6ca70, 29) == read(rebuilt, names['_builtin_group_boundaries'], 29)
def cstring(image, address):
    result = bytearray()
    while read(image, address, 1) != b'\0':
        result += read(image, address, 1)
        address += 1
    return bytes(result)
for index in range(51):
    old = struct.unpack('<I', read(original, 0x6c9a0 + 4*index, 4))[0]
    new = struct.unpack('<I', read(rebuilt, names['_builtin_definitions'] + 16*index, 4))[0]
    assert cstring(original, old) == cstring(rebuilt, new+1), ('builtin suffix', index)

print('%d state defaults/aliases, %d scalar options, 21 option targets, 17 pragma rules and 5 hash callbacks match; 51 builtin suffixes/groups match.' % (len(STATE), option_scalar_count))
