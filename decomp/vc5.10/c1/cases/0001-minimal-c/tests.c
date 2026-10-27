#include "compiler-state.h"
#include "packing.h"
#include "case.h"
#include <recon.h>

static int packing_alignment(void)
{
    unsigned storage[6];
    const char tag[] = "owned tag";
    CompilerPackingFrame predecessor = { NULL, 8, tag };
    CompilerPacking stack = { &predecessor, NULL };
    CompilerPackingFrame *first, *second;
    CompilerArena *arena = &c1_bss.arenas[0];

    arena->cursor = (BYTE *)storage;
    arena->committed_limit = (BYTE *)(storage + 6);
    c1_data.options.packing_stack = &stack;
    first = push_packing(0, NULL);
    if (first != (CompilerPackingFrame *)storage || first->previous != &predecessor ||
        first->alignment != 8 || first->tag || stack.active != first ||
        arena->cursor != (BYTE *)(storage + 3)) return 0;
    second = push_packing(4, tag);
    return second == (CompilerPackingFrame *)(storage + 3) &&
        second->previous == first && second->alignment == 4 && second->tag == tag &&
        stack.active == second && stack.free_frames == NULL &&
        arena->cursor == (BYTE *)(storage + 6) && first->alignment == 8 &&
        first->tag == NULL && first->previous == &predecessor &&
        predecessor.previous == NULL && predecessor.alignment == 8 && predecessor.tag == tag;
}

extern int c1_test_input_factory(void);
extern int c1_test_null_debug_factory(void);
extern int c1_test_parser_reset_extent(void);
extern int c1_test_parameter_scope_layout(void);

extern void __cdecl normalize_frontend_options(void);
static int processor_instruction_mask(void)
{
    c1_bss.processor_instruction_mask = 0xffffffff;
    c1_data.options.processor = 0;
    c1_data.options.minimal_rebuild = 0;
    normalize_frontend_options();
    if (c1_bss.processor_instruction_mask != 1) return 0;
    c1_data.options.processor = 4;
    c1_data.options.minimal_rebuild = 1;
    normalize_frontend_options();
    return c1_bss.processor_instruction_mask == 0x50;
}
extern int c1_test_browser_factory(void);
extern int c1_test_backend_getter(void);
extern void __cdecl initialize_predefined_tokens(void);
static int predefined_token_prototypes(void)
{
    BYTE storage[244 * 32 + 2];
    CompilerTokenPrototype retained;
    CompilerPool *pool = &c1_bss.pools[6];
    unsigned kind, count = 0, i;
    BYTE *next = storage + 1;
    /* Test-owned nonzero bytes prove the initializer touches only its two
     * defined fields. They are not an original-data equivalence claim. */
    for (i = 0; i < sizeof(storage); ++i) storage[i] = 0xa5;
    for (kind = 0; kind < 245; ++kind)
        c1_data.predefined_tokens[kind] = &retained;
    pool->cursor = next;
    pool->remaining = 244 * 32;
    initialize_predefined_tokens();
    if (c1_data.predefined_tokens[0] != &retained ||
        c1_data.predefined_tokens[6] != NULL ||
        !c1_data.predefined_tokens[1] || !c1_data.predefined_tokens[244]) return 0;
    for (kind = 1; kind < 245; ++kind) {
        CompilerTokenPrototype *token = c1_data.predefined_tokens[kind];
        if (!token) continue;
        if ((BYTE *)token != next || token->kind != kind || token->tag != kind)
            return 0;
        for (i = 0; i < 32; ++i)
            if (i != 0 && i != 8 && next[i] != 0xa5) return 0;
        next += 32;
        ++count;
    }
    return count == 167 && pool->cursor == next &&
        pool->remaining == (244 - 167) * 32 && storage[0] == 0xa5 &&
        storage[sizeof(storage) - 1] == 0xa5;
}
extern void __cdecl initialize_keyword_table(void);
static int keyword_storage(void)
{
    DWORD storage[2050];
    CompilerArena *arena = &c1_bss.arenas[0];
    BYTE *begin = (BYTE *)(storage + 1);
    unsigned i;
    for (i = 0; i < 2050; ++i) storage[i] = 0xa5a5a5a5;
    c1_data.name_buckets = NULL;
    arena->cursor = begin;
    arena->committed_limit = begin + 8192;
    initialize_keyword_table();
    if ((BYTE *)c1_data.name_buckets != begin || arena->cursor != begin + 8192)
        return 0;
    /* An existing table returns without touching the arena or its bytes. */
    arena->cursor = begin + 4;
    initialize_keyword_table();
    if ((BYTE *)c1_data.name_buckets != begin || arena->cursor != begin + 4)
        return 0;
    for (i = 0; i < 2050; ++i) if (storage[i] != 0xa5a5a5a5) return 0;
    return 1;
}
extern int c1_test_identifier_interning(void);
static int output_filename_constructor(void)
{
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    unsigned i;
    for (i = 0; i < MAX_PATH; ++i) buffer->filename[i] = 'x';
    buffer->file_descriptor = 123;
    buffer->owned_debug_name = (char *)buffer;
    memset(buffer->unmodeled_124, 0xa5, sizeof(buffer->unmodeled_124));
    initialize_output_buffer_124c1();
    if (buffer->filename[0] || buffer->file_descriptor != -1) return 0;
    if (buffer->owned_debug_name != (char *)buffer) return 0;
    for (i = 0; i < sizeof(buffer->unmodeled_124); ++i)
        if (buffer->unmodeled_124[i] != 0xa5) return 0;
    for (i = 1; i < MAX_PATH; ++i)
        if (buffer->filename[i] != 'x') return 0;
    return 1;
}
extern CompilerIntegerConstant *__cdecl create_integer_constant(int);
extern CompilerIntegerConstant *__cdecl make_integer_constant(int);
static int integer_nodes(void)
{
    DWORD storage[20];
    CompilerType type;
    CompilerIntegerConstant *node;
    unsigned i;
    for (i = 0; i < 20; ++i) storage[i] = 0xa5a5a5a5;
    c1_data.expression_arena_kind = 3;
    c1_data.builtin_types.type_0063 = &type;
    c1_bss.arenas[3].cursor = (BYTE *)storage + 4;
    c1_bss.arenas[3].committed_limit = (BYTE *)storage + sizeof(storage);
    node = create_integer_constant(-17);
    if ((BYTE *)node != (BYTE *)storage + 4 || node->kind != 101 ||
        node->flags != 3 || node->type != &type || node->integer != -17 ||
        storage[3] != 0xa5a5a5a5 || storage[4] != 0xa5a5a5a5 ||
        ((BYTE *)node)[2] != 0xa5 || ((BYTE *)node)[3] != 0xa5) return 0;
    node = make_integer_constant(0);
    if ((BYTE *)node != (BYTE *)storage + 28 || node->flags != 0x83 ||
        node->kind != 101 || node->integer || node->type != &type) return 0;
    node = make_integer_constant(7);
    return (BYTE *)node == (BYTE *)storage + 52 && node->flags == 3 &&
        node->integer == 7 && c1_bss.arenas[3].cursor == (BYTE *)storage + 76 &&
        storage[0] == 0xa5a5a5a5 && storage[19] == 0xa5a5a5a5;
}
extern void __cdecl emit_pending_line_record(void);
static int pending_line_record(void)
{
    BYTE bytes[16];
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    buffer->data = bytes;
    buffer->cursor = bytes;
    buffer->capacity = sizeof(bytes);
    buffer->good = 7;
    c1_bss.ir_record_count = 67;
    c1_bss.pending_line_delta = 5;
    c1_data.last_line_delta = 9;
    emit_pending_line_record();
    if (buffer->cursor != bytes + 3 || buffer->good != 1 ||
        bytes[0] != 0x4f || bytes[1] != 0x24 || bytes[2] != 5 ||
        c1_data.last_line_delta != 5) return 0;
    emit_pending_line_record();
    c1_bss.pending_line_delta = 0;
    emit_pending_line_record();
    if (buffer->cursor != bytes + 3 || c1_data.last_line_delta != 5) return 0;
    c1_bss.pending_line_delta = -2;
    emit_pending_line_record();
    return buffer->cursor == bytes + 8 && bytes[3] == 0x4f && bytes[4] == 0x24 &&
        bytes[5] == 0x80 && bytes[6] == 0xfe && bytes[7] == 0xff &&
        c1_data.last_line_delta == -2 && c1_bss.ir_record_count == 67;
}
extern int c1_test_binary_node_extent(void);
extern int c1_test_block_scope_extent(void);
extern int c1_test_based_literal_boundaries(void);
extern int c1_test_output_short(void);
extern int c1_test_output_index(void);
extern int c1_test_output_small(void);
extern int c1_test_output_ensure(void);
extern int c1_test_output_grow(void);
extern int c1_test_output_flush_suppression(void);
extern void __cdecl initialize_type_table(void);
static int type_table_storage(void)
{
    DWORD storage[4098];
    CompilerArena *arena = &c1_bss.arenas[0];
    unsigned i;
    for (i = 0; i < 4098; ++i) storage[i] = 0xa5a5a5a5;
    arena->cursor = (BYTE *)(storage + 1);
    arena->committed_limit = (BYTE *)(storage + 4097);
    initialize_type_table();
    if ((void *)c1_data.type_buckets != storage + 1 ||
        (void *)c1_data.type_shape_buckets != storage + 2049 ||
        (void *)c1_data.type_list_buckets != storage + 3073 ||
        arena->cursor != (BYTE *)(storage + 4097)) return 0;
    for (i = 0; i < 4098; ++i) if (storage[i] != 0xa5a5a5a5) return 0;
    return 1;
}
extern CompilerType *__cdecl intern_type(CompilerType *);
static int type_interning(void)
{
    CompilerTypeEntry *buckets[2048] = { 0 };
    DWORD storage[17];
    CompilerType key = { 0, 0, NULL, 0, 37 };
    CompilerType *first, *second, *third;
    CompilerPool *pool = &c1_bss.pools[0];
    unsigned i;
    for (i = 0; i < 17; ++i) storage[i] = 0xa5a5a5a5;
    c1_data.type_buckets = buckets;
    c1_data.options.debug_info = 0;
    pool->cursor = (BYTE *)(storage + 1);
    pool->remaining = 60;
    first = intern_type(&key);
    key.debug_index = 99;
    if (first->debug_index != 37 || intern_type(&key) != first ||
        pool->remaining != 40) return 0;
    key.flags = 2048; /* Same bucket, distinct semantic key. */
    second = intern_type(&key);
    if (second == first || buckets[0]->next != (CompilerTypeEntry *)(storage + 1) ||
        second->flags != 2048 || second->debug_index != 99) return 0;
    key.flags = 0;
    if (intern_type(&key) != first) return 0;
    key.attributes = 1;
    c1_data.options.debug_info = 1;
    third = intern_type(&key);
    return third != first && third != second && third->debug_index == 0 &&
        third->attributes == 1 && pool->remaining == 0 &&
        pool->cursor == (BYTE *)(storage + 16) && storage[0] == 0xa5a5a5a5 &&
        storage[16] == 0xa5a5a5a5;
}
extern void __cdecl initialize_global_scope(void);
static int global_symbol_arena(void)
{
    unsigned owned_marker = 37;
    c1_bss.saved_symbol_arenas = (struct CompilerSymbolArenaFrame *)&owned_marker;
    c1_bss.symbol_arena_kind = 5;
    c1_bss.ir_record_count = 83;
    initialize_global_scope();
    return c1_bss.symbol_arena_kind == 2 && !c1_bss.saved_symbol_arenas &&
        c1_bss.ir_record_count == 83 && owned_marker == 37;
}
extern int c1_test_returning_interner(void);
extern int c1_test_translation_unit(void);
extern int c1_test_abstract_symbol_reuse(void);
extern int c1_test_symbol_constructor(void);
extern struct CompilerTypeShape *__cdecl create_type_shape(unsigned, unsigned, const unsigned *);
static int type_shape_allocation(void)
{
    DWORD storage[5];
    unsigned i, value = 37;
    struct CompilerTypeShape *shape;
    for (i = 0; i < 5; ++i) storage[i] = 0xa5a5a5a5;
    c1_bss.arenas[3].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[3].committed_limit = (BYTE *)(storage + 4);
    shape = create_type_shape(0x12345, 0x23456, &value);
    return (void *)shape == storage + 1 && shape->flags == 0x2345 &&
        shape->attributes == 0x3456 && shape->value == 37 &&
        (unsigned)shape->base == 0xa5a5a5a5 &&
        c1_bss.arenas[3].cursor == (BYTE *)(storage + 4) &&
        storage[0] == 0xa5a5a5a5 && storage[4] == 0xa5a5a5a5;
}
extern CompilerTypeList *__cdecl get_empty_type_list(void);
static int cached_empty_type_list(void)
{
    DWORD storage[5];
    CompilerTypeList *list;
    unsigned i;
    for (i = 0; i < 5; ++i) storage[i] = 0xa5a5a5a5;
    c1_data.empty_type_list = NULL;
    c1_bss.arenas[0].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[0].committed_limit = (BYTE *)(storage + 4);
    list = get_empty_type_list();
    if ((void *)list != storage + 1 || c1_data.empty_type_list != list ||
        list->count || storage[2] || storage[3] != 0xa5a5a5a5 ||
        c1_bss.arenas[0].cursor != (BYTE *)(storage + 4)) return 0;
    c1_bss.arenas[0].cursor = (BYTE *)(storage + 2);
    return get_empty_type_list() == list &&
        c1_bss.arenas[0].cursor == (BYTE *)(storage + 2) &&
        storage[0] == 0xa5a5a5a5 && storage[4] == 0xa5a5a5a5;
}
extern int c1_test_append_type_shape(void);
extern struct CompilerTypeShape *__cdecl intern_type_shape(struct CompilerTypeShape *);
static int interned_type_shapes(void)
{
    struct CompilerTypeShapeEntry *buckets[1024];
    struct CompilerTypeShape first, collision, *a, *b;
    DWORD storage[10];
    unsigned i, hash;
    for (i = 0; i < 1024; ++i) buckets[i] = NULL;
    for (i = 0; i < 10; ++i) storage[i] = 0xa5a5a5a5;
    first.flags = 4;
    first.attributes = 0x10;
    first.base = NULL;
    first.value = 1;
    collision = first;
    collision.value = 4097;
    c1_data.type_shape_buckets = buckets;
    c1_bss.pools[1].cursor = (BYTE *)(storage + 1);
    c1_bss.pools[1].remaining = 32;
    a = intern_type_shape(&first);
    b = intern_type_shape(&collision);
    hash = 4 + 0x10;
    return a == (struct CompilerTypeShape *)(storage + 2) &&
        b == (struct CompilerTypeShape *)(storage + 6) && a != b &&
        a->value == 1 && b->value == 4097 && a->flags == 4 &&
        a->attributes == 0x10 && !a->base &&
        (void *)buckets[hash] == storage + 5 &&
        (void *)buckets[hash]->next == storage + 1 &&
        !buckets[hash]->next->next && intern_type_shape(&first) == a &&
        intern_type_shape(NULL) == NULL && c1_bss.pools[1].remaining == 0 &&
        c1_bss.pools[1].cursor == (BYTE *)(storage + 9) &&
        storage[0] == 0xa5a5a5a5 && storage[9] == 0xa5a5a5a5;
}
extern unsigned __cdecl pack_option_flags(void);
static int packed_option_bits(void)
{
    c1_data.normalized_stack_checks = 0;
    c1_data.options.assume_no_alias = 0;
    c1_data.options.assume_no_global_alias = 0;
    c1_data.options.optimize_size = 0;
    c1_data.options.global_optimize = 0;
    c1_data.options.precise_float = 0;
    c1_data.normalized_oq = 0;
    c1_data.options.option_ov = 0;
    c1_data.options.omit_frame_pointer = 0;
    if (pack_option_flags()) return 0;
    c1_data.normalized_stack_checks = -7;
    c1_data.options.optimize_size = 2;
    c1_data.options.omit_frame_pointer = -1;
    return pack_option_flags() == 0x80800800U;
}
extern void __cdecl make_incremental_database_path(char *);
static int incremental_path_extent(void)
{
    unsigned char owned[32];
    unsigned i;
    memset(owned, 0xa5, sizeof(owned));
    memcpy(c1_data.options.database_path, "alpha.beta.gamma", 17);
    make_incremental_database_path((char *)owned + 1);
    if (memcmp(owned + 1, "alpha.beta.idb", 14)) return 0;
    for (i = 0; i < sizeof(owned); ++i)
        if ((i == 0 || i >= 15) && owned[i] != 0xa5) return 0;
    memset(owned, 0x96, sizeof(owned));
    memcpy(c1_data.options.database_path, "plain", 6);
    make_incremental_database_path((char *)owned + 1);
    if (memcmp(owned + 1, "plain\0.idb", 10)) return 0;
    for (i = 0; i < sizeof(owned); ++i)
        if ((i == 0 || i >= 11) && owned[i] != 0x96) return 0;
    return 1;
}
extern unsigned __cdecl primitive_type_size(WORD, unsigned);
static int primitive_width_relation(void)
{
    static const unsigned expected[16] = {0,1,2,4,4,2,4,8,8,1,2,4,8,16,0,0};
    unsigned i;
    for (i = 0; i < 16; ++i)
        if (primitive_type_size((WORD)i, 0) != expected[i] ||
            primitive_type_size((WORD)(0x40 | i), 0) != expected[i] ||
            primitive_type_size((WORD)(0x200 | i), 0) != 4 ||
            primitive_type_size((WORD)(0x1000 | i), 0) != i)
            return 0;
    return 1;
}
static int type_arena_snapshot(void)
{
    CompilerInitializedState *saved;
    CompilerIrSymbol owned_symbols[2];
    CompilerSourceFrame owned_frames[3];
    unsigned i;
    if (c1_data.function_directory_head || c1_data.function_directory_current || c1_data.function_directory_index != 19 || c1_data.function_debug_handle || c1_data.function_cleanup_flags || c1_data.debug_symbol_index || c1_data.function_scope_cache || c1_data.source_exhausted || c1_data.final_input_token_emitted || c1_data.omit_external_names || c1_data.pch_type_record_value || c1_data.pch_record_name || c1_data.global_options || c1_data.pch_expression_stream_open || c1_data.pch_primary_stream_open || c1_data.pch_secondary_stream_open || c1_data.pch_cleanup_done || c1_data.saved_context_records || c1_data.saved_context_count || c1_bss.function_cleanup_context || c1_bss.token_input_finished || c1_bss.pending_scope_symbols || c1_data.try_depth || c1_data.finally_depth || c1_data.exception_filter_depth || c1_bss.case_label || c1_bss.function_stack_check_reported || c1_bss.parser_early_result || c1_data.label_symbols || c1_bss.function_entry_label || c1_data.line_range_warning_enabled != 1 || c1_data.function_record_flags || c1_bss.function_expression_offset || c1_bss.function_symbol_offset || c1_bss.function_output_mode || c1_bss.parameter_symbol_count || c1_bss.lexical_skip_mode || c1_bss.parameter_scope_value || c1_bss.function_storage_class || c1_data.function_label_state_first != -1 || c1_data.function_label_state_second != -1 || c1_bss.function_reset_value || c1_bss.nonil_type_index || c1_data.parser_flags || c1_data.function_return_type || c1_data.function_label_depth || c1_data.local_symbol_limit != 0x3fffff || c1_data.function_reference_epoch != 1 || c1_data.disable_inline_records || c1_data.function_context || c1_bss.parameter_storage_class || c1_bss.parameter_scope_initial_value || c1_bss.unevaluated_expression_depth || c1_bss.parameter_nesting || c1_bss.declaration_depth || c1_bss.declaration_control[0].flags || c1_bss.type_description_depth || c1_data.lexer_flags || c1_data.active_parser || c1_bss.parser_value.declaration_flags || c1_bss.browser_context_value || c1_data.debug_module_kind != 2 || c1_data.source_frames || c1_data.root_source_frame || c1_data.free_source_frames ||
        c1_data.source_descriptor != -1 || c1_data.conditional_depth != -1 ||
        c1_bss.source_cursor || c1_data.type_arena_kind != 3 ||
        c1_data.inline_depth != 8 || c1_data.inline_recursion ||
        c1_bss.forced_include_name || c1_bss.browser_stream || c1_data.browser_last_line || c1_data.browser_absolute_line || c1_data.browser_output_stream || c1_data.browser_suppressed || c1_data.browser_header_position || c1_data.browser_generation ||
        c1_data.pragma_restore_options != 0x03fff800 || c1_data.pragma_disable_options != 0x037eb800 ||
        c1_data.anonymous_identifier_count != 1 || c1_bss.runtime_initializer_pending || c1_data.segment_symbol_cache_index != 9 || c1_data.declaration_context_map ||
        c1_data.record_sizes.known.segment_symbol_bytes != 52 ||
        c1_data.declaration_symbol_cache_index != 9 || c1_data.member_symbol_cache_index != 9 || c1_data.compiler_messages_initialized ||
        c1_data.record_sizes.known.declaration_symbol_bytes != 52 ||
        c1_data.record_sizes.known.bitfield_symbol_bytes != 52 ||
        c1_data.record_sizes.known.named_parameter_symbol_bytes != 52 ||
        c1_data.record_sizes.known.intrinsic_symbol_bytes != 52 ||
        c1_data.record_sizes.known.global_symbol_bytes != 52 ||
        c1_data.record_sizes.known.abstract_symbol_bytes != 52 ||
        memcmp(c1_data.predefined_time_text, "......Unknown date......\0", 26) ||
        c1_data.source_include_depth != -1 ||
        c1_data.macro_reset_value != 0 || c1_data.emit_global_locations != 0 ||
        c1_data.global_symbol_cache_index != 9 ||
        c1_data.anonymous_global_symbol || c1_data.local_size_symbol ||
        c1_data.intrinsic_symbol_cache_index != 9 || c1_data.next_symbol_record_id != 16)
        return 0;
    for (i = 0; i < 256; ++i)
        if (c1_bss.incremental_database_path[i]) return 0;
    c1_data.abstract_symbol_free = &owned_symbols[0].binding;
    memset(c1_data.predefined_time_text, 0x37, 26);
    c1_data.source_include_depth = 13;
    c1_data.browser_last_line = 239;
    c1_data.browser_absolute_line = 17;
    c1_data.browser_output_stream = (FILE *)&owned_symbols[0];
    c1_data.browser_suppressed = 67;
    c1_data.browser_header_position = 113;
    c1_data.browser_generation = 137;
    c1_data.macro_reset_value = 83;
    c1_data.emit_global_locations = 37;
    c1_data.debug_module_kind = 37;
    c1_data.lexer_flags = 0xa537U;
    c1_data.active_parser = (struct CompilerParser *)&owned_symbols[0];
    c1_data.source_frames = &owned_frames[0];
    c1_data.root_source_frame = &owned_frames[1];
    c1_data.free_source_frames = &owned_frames[2];
    c1_data.source_descriptor = 41;
    c1_data.conditional_depth = 13;
    c1_data.type_arena_kind = 2;
    c1_data.declaration_symbol_cache_index = 13;
    c1_data.member_symbol_cache_index = 37;
    unsigned catalog_index = 0;
    while (catalog_index < C1_COMPILER_MESSAGE_COUNT &&
           c1_data.compiler_messages[catalog_index].id != C1_TEXT_FUNCTION_THROUGH_POINTER)
        ++catalog_index;
    if (catalog_index == C1_COMPILER_MESSAGE_COUNT ||
        c1_data.compiler_messages[catalog_index].text ||
        c1_data.compiler_messages[86].id || c1_data.compiler_messages[86].text) return 0;
    c1_data.compiler_messages[catalog_index].text = (char *)&owned_symbols[0];
    c1_data.compiler_messages_initialized = 37;
    c1_data.record_sizes.known.declaration_symbol_bytes = 47;
    c1_data.record_sizes.known.bitfield_symbol_bytes = 49;
    c1_data.record_sizes.known.named_parameter_symbol_bytes = 41;
    c1_data.record_sizes.known.intrinsic_symbol_bytes = 37;
    c1_data.record_sizes.known.global_symbol_bytes = 41;
    c1_data.record_sizes.known.abstract_symbol_bytes = 43;
    c1_data.record_sizes.known.segment_symbol_bytes = 45;
    c1_data.segment_symbol_cache_index = 11;
    c1_data.anonymous_identifier_count = 53;
    c1_data.inline_depth = 4;
    c1_data.inline_recursion = 1;
    c1_data.pragma_restore_options = 0x12345678;
    c1_data.pragma_disable_options = 0x87654321;
    c1_data.declaration_context_map = (CompilerSymbolMap *)&owned_symbols[0];
    c1_data.global_symbol_cache_index = 17;
    c1_data.anonymous_global_symbol = &owned_symbols[0];
    c1_data.local_size_symbol = &owned_symbols[1];
    c1_data.intrinsic_symbol_cache_index = 7;
    c1_data.next_symbol_record_id = 83;
    c1_data.label_symbols = &owned_symbols[0];
    c1_data.line_range_warning_enabled = 37;
    c1_data.function_record_flags = 0x12345678;
    c1_data.function_label_state_first = 37;
    c1_data.function_label_state_second = 43;
    c1_data.parser_flags = 0x1234;
    c1_data.function_return_type = (CompilerType *)&owned_symbols[0];
    c1_data.function_label_depth = 17;
    c1_data.local_symbol_limit = 0x12345;
    c1_data.function_reference_epoch = 37;
    c1_data.disable_inline_records = 43;
    c1_data.function_context = &owned_symbols[0];
    c1_data.function_directory_head = (struct CompilerFunctionDirectoryBlock *)&owned_symbols[0];
    c1_data.function_directory_current = (struct CompilerFunctionDirectoryBlock *)&owned_symbols[1];
    c1_data.function_directory_index = 7;
    c1_data.function_debug_handle = 37;
    c1_data.function_cleanup_flags = 71;
    c1_data.debug_symbol_index = 79;
    c1_data.source_exhausted = 17;
    c1_data.final_input_token_emitted = 29;
    c1_data.omit_external_names = 37;
    c1_data.pch_type_record_value = 0x1234;
    c1_data.pch_record_name = (const char *)&owned_symbols[0];
    c1_data.global_options = (struct CompilerGlobalOption *)&owned_symbols[1];
    c1_data.pch_expression_stream_open = 17;
    c1_data.pch_primary_stream_open = 19;
    c1_data.pch_secondary_stream_open = 23;
    c1_data.pch_cleanup_done = 29;
    c1_data.saved_context_records = (struct CompilerSavedContextRecord *)&owned_symbols[0];
    c1_data.saved_context_count = 17;
    c1_data.function_scope_cache = &owned_symbols[1];
    if (initialize_compiler_pass()) return 0;
    saved = c1_bss.saved_data;
    if (saved->function_directory_head != (struct CompilerFunctionDirectoryBlock *)&owned_symbols[0] ||
        saved->function_directory_current != (struct CompilerFunctionDirectoryBlock *)&owned_symbols[1] ||
        saved->function_directory_index != 7 || saved->function_debug_handle != 37 || saved->function_cleanup_flags != 71 || saved->debug_symbol_index != 79 || saved->function_scope_cache != &owned_symbols[1] || saved->source_exhausted != 17 || saved->final_input_token_emitted != 29 || saved->omit_external_names != 37 || saved->pch_type_record_value != 0x1234 || saved->pch_record_name != (const char *)&owned_symbols[0] || saved->global_options != (struct CompilerGlobalOption *)&owned_symbols[1] || saved->pch_expression_stream_open != 17 || saved->pch_primary_stream_open != 19 || saved->pch_secondary_stream_open != 23 || saved->pch_cleanup_done != 29 || saved->saved_context_records != (struct CompilerSavedContextRecord *)&owned_symbols[0] || saved->saved_context_count != 17) return 0;
    if (saved->label_symbols != &owned_symbols[0] || saved->line_range_warning_enabled != 37 || saved->function_record_flags != 0x12345678 || saved->function_label_state_first != 37 || saved->function_label_state_second != 43 || saved->parser_flags != 0x1234 || saved->function_return_type != (CompilerType *)&owned_symbols[0] || saved->function_label_depth != 17 || saved->local_symbol_limit != 0x12345 || saved->function_reference_epoch != 37 || saved->disable_inline_records != 43 || saved->function_context != &owned_symbols[0] || saved->lexer_flags != 0xa537U || saved->active_parser != (struct CompilerParser *)&owned_symbols[0] || saved->debug_module_kind != 37 || saved->source_frames != &owned_frames[0] ||
        saved->root_source_frame != &owned_frames[1] ||
        saved->free_source_frames != &owned_frames[2] ||
        saved->source_descriptor != 41 || saved->conditional_depth != 13 ||
        saved->emit_global_locations != 37 || saved->type_arena_kind != 2 ||
        saved->declaration_symbol_cache_index != 13 || saved->member_symbol_cache_index != 37 || saved->compiler_messages_initialized != 37 || saved->compiler_messages[catalog_index].text != (char *)&owned_symbols[0] ||
        saved->record_sizes.known.declaration_symbol_bytes != 47 ||
        saved->record_sizes.known.bitfield_symbol_bytes != 49 ||
        saved->record_sizes.known.named_parameter_symbol_bytes != 41 ||
        saved->record_sizes.known.intrinsic_symbol_bytes != 37 ||
        saved->record_sizes.known.global_symbol_bytes != 41 ||
        saved->record_sizes.known.abstract_symbol_bytes != 43 ||
        saved->record_sizes.known.segment_symbol_bytes != 45 ||
        saved->inline_depth != 4 || saved->inline_recursion != 1 ||
        saved->pragma_restore_options != 0x12345678 || saved->pragma_disable_options != 0x87654321 ||
        saved->segment_symbol_cache_index != 11 || saved->anonymous_identifier_count != 53 ||
        saved->declaration_context_map != (CompilerSymbolMap *)&owned_symbols[0] ||
        saved->global_symbol_cache_index != 17 ||
        saved->anonymous_global_symbol != &owned_symbols[0] ||
        saved->local_size_symbol != &owned_symbols[1]) return 0;
    if (saved->abstract_symbol_free != &owned_symbols[0].binding) return 0;
    /* The cache gaps remain unmodeled. Distinct owned storage
     * sentinels detect copying it along with its separately known head9. */
    memset(saved->symbol_cache.known.unmodeled_00, 0xa5, sizeof(saved->symbol_cache.known.unmodeled_00));
    memset(c1_data.symbol_cache.known.unmodeled_00, 0x5a, sizeof(c1_data.symbol_cache.known.unmodeled_00));
    c1_data.abstract_symbol_free = &owned_symbols[1].binding;
    /* Owned representation sentinels are not original table defaults.
     * Restore must not transfer any of these14 unknown slots. */
    for (i = 0; i < 26; ++i) {
        if (i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 6 ||
            i == 9 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14) continue;
        saved->record_sizes.indexed[i] = 0xa5000000 + i;
        c1_data.record_sizes.indexed[i] = 0x5a000000 + i;
    }
    memset(c1_data.predefined_time_text, 0x61, 26);
    c1_data.source_include_depth = 29;
    c1_data.browser_last_line = 251;
    c1_data.browser_absolute_line = 19;
    c1_data.browser_output_stream = (FILE *)&owned_symbols[1];
    c1_bss.browser_last_symbol = &owned_symbols[1];
    c1_data.browser_suppressed = 71;
    c1_data.browser_header_position = 127;
    c1_data.browser_generation = 149;
    c1_data.macro_reset_value = 97;
    c1_data.emit_global_locations = 61;
    c1_data.debug_module_kind = 61;
    c1_data.lexer_flags = 0x5a43U;
    c1_data.active_parser = (struct CompilerParser *)&owned_symbols[1];
    c1_data.label_symbols = &owned_symbols[1];
    c1_bss.function_entry_label = (struct CompilerParserNode *)&owned_symbols[0];
    c1_data.line_range_warning_enabled = 0;
    c1_data.function_record_flags = 0xa596a596;
    c1_bss.function_expression_offset = 37;
    c1_bss.function_symbol_offset = 43;
    c1_data.function_label_state_first = 61;
    c1_data.function_label_state_second = 67;
    c1_bss.function_output_mode = 43;
    c1_bss.parameter_symbol_count = 67;
    c1_bss.lexical_skip_mode = 37;
    c1_bss.case_label = (struct CompilerParserNode *)&c1_bss.literal;
    c1_bss.function_stack_check_reported = 53;
    c1_bss.parser_early_result = 27;
    c1_bss.parameter_scope_value = 0x123;
    c1_bss.function_storage_class = 0x456;
    c1_data.parser_flags = 0xa596;
    c1_data.function_return_type = (CompilerType *)&owned_symbols[1];
    c1_data.function_label_depth = 23;
    c1_bss.browser_stream = (FILE *)&owned_symbols[1];
    c1_bss.function_reset_value = 0xabcdef;
    c1_bss.nonil_type_index = 0x456789;
    c1_data.local_symbol_limit = 0x56789;
    c1_data.function_reference_epoch = 61;
    c1_data.disable_inline_records = 67;
    c1_data.function_context = &owned_symbols[1];
    c1_bss.parser_value.declaration_flags = 0x5a96a587;
    c1_data.source_frames = &owned_frames[2];
    c1_data.root_source_frame = &owned_frames[0];
    c1_data.free_source_frames = &owned_frames[1];
    c1_data.source_descriptor = 67;
    c1_data.conditional_depth = 29;
    c1_bss.source_cursor = (BYTE *)owned_frames;
    memset(c1_bss.incremental_database_path, 0xa5, 256);
    c1_bss.browser_context_value = 0xa596;
    c1_bss.unevaluated_expression_depth = 43;
    c1_bss.parameter_storage_class = 37;
    c1_bss.parameter_scope_initial_value = 0x12345678;
    c1_bss.parameter_nesting = 37;
    c1_bss.declaration_control[1].flags = 0x5a;
    c1_bss.declaration_depth = 41;
    c1_bss.declaration_control[0].flags = 0xa5;
    /* Native24B declaration representation maps the five original20B fields;
     * its extra member-pointer word is representation, not original padding. */
    memset(c1_bss.declaration_specs[0], 0xa5, sizeof(c1_bss.declaration_specs[0]));
    c1_bss.pending_scope_symbols = (struct CompilerPendingScopeSymbol *)&owned_symbols[0];
    c1_data.function_directory_head = NULL;
    c1_data.function_directory_current = NULL;
    c1_data.function_directory_index = 13;
    c1_data.function_debug_handle = 43;
    c1_data.function_cleanup_flags = 97;
    c1_data.debug_symbol_index = 83;
    c1_data.source_exhausted = 23;
    c1_data.final_input_token_emitted = 31;
    c1_data.omit_external_names = 41;
    c1_data.pch_type_record_value = 0x5678;
    c1_data.pch_expression_stream_open = 31;
    c1_data.pch_primary_stream_open = 37;
    c1_data.pch_secondary_stream_open = 41;
    c1_data.pch_cleanup_done = 43;
    c1_data.saved_context_records = NULL;
    c1_data.saved_context_count = 23;
    c1_data.pch_record_name = NULL;
    c1_data.global_options = NULL;
    c1_bss.token_input_finished = 19;
    c1_bss.dereferenced_shape_value = 0x87654321;
    c1_data.function_scope_cache = &owned_symbols[0];
    c1_bss.function_cleanup_context = &owned_symbols[1];
    c1_data.try_depth = 17;
    c1_data.finally_depth = 23;
    c1_data.exception_filter_depth = 31;
    c1_bss.type_description_depth = 37;
    c1_bss.type_descriptions[1].flags = 0xa5a5;
    c1_bss.type_descriptions[1].attributes = 0x5a5a;
    c1_bss.type_descriptions[1].shape = (struct CompilerTypeShape *)&owned_symbols[0];
    c1_bss.type_descriptions[1].auxiliary = 83;
    c1_bss.type_descriptions[1].debug_index = 97;
    saved->first_call = 0;
    c1_data.declaration_symbol_cache_index = 17;
    c1_data.member_symbol_cache_index = 43;
    c1_data.compiler_messages_initialized = 43;
    c1_data.compiler_messages[catalog_index].text = (char *)&owned_symbols[1];
    c1_data.record_sizes.known.declaration_symbol_bytes = 73;
    c1_data.record_sizes.known.bitfield_symbol_bytes = 79;
    c1_data.record_sizes.known.named_parameter_symbol_bytes = 47;
    c1_data.record_sizes.known.intrinsic_symbol_bytes = 61;
    c1_data.record_sizes.known.global_symbol_bytes = 67;
    c1_data.record_sizes.known.abstract_symbol_bytes = 71;
    c1_data.record_sizes.known.segment_symbol_bytes = 77;
    c1_data.segment_symbol_cache_index = 3;
    c1_data.anonymous_identifier_count = 79;
    c1_data.inline_depth = 31;
    c1_data.inline_recursion = 27;
    c1_data.pragma_restore_options = 0x11223344;
    c1_data.pragma_disable_options = 0x55667788;
    c1_bss.runtime_initializer_pending = 37;
    c1_bss.forced_include_name = "dirty forced include";
    c1_data.declaration_context_map = (CompilerSymbolMap *)&owned_symbols[1];
    c1_data.global_symbol_cache_index = 3;
    c1_data.anonymous_global_symbol = NULL;
    c1_data.local_size_symbol = NULL;
    c1_data.intrinsic_symbol_cache_index = 3;
    c1_data.next_symbol_record_id = 95;
    c1_data.type_arena_kind = 5;
    c1_bss.declaration_scope_active = 1;
    c1_pass_mode = 1;
    if (initialize_compiler_pass()) return 0;
    if (c1_data.abstract_symbol_free != &owned_symbols[0].binding ||
        saved->abstract_symbol_free != &owned_symbols[0].binding) return 0;
    for (i = 0; i < sizeof(saved->symbol_cache.known.unmodeled_00); ++i)
        if (saved->symbol_cache.known.unmodeled_00[i] != 0xa5 ||
            c1_data.symbol_cache.known.unmodeled_00[i] != 0x5a) return 0;
    for (i = 0; i < 26; ++i) {
        if (i == 1 || i == 2 || i == 3 || i == 4 || i == 5 || i == 6 ||
            i == 9 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14) continue;
        if (c1_data.record_sizes.indexed[i] != 0x5a000000 + i ||
            saved->record_sizes.indexed[i] != 0xa5000000 + i) return 0;
    }
    for (i = 0; i < 256; ++i)
        if (c1_bss.incremental_database_path[i]) return 0;
    for (i = 0; i < 26; ++i)
        if (c1_data.predefined_time_text[i] != 0x37 ||
            saved->predefined_time_text[i] != 0x37) return 0;
    for (i = 0; i < sizeof(c1_bss.declaration_specs[0]); ++i)
        if (c1_bss.declaration_specs[0][i]) return 0;
    if (c1_bss.forced_include_name || c1_bss.browser_stream || c1_bss.browser_last_symbol) return 0;
    if (c1_bss.runtime_initializer_pending || c1_bss.dereferenced_shape_value || c1_bss.token_input_finished || c1_bss.case_label || c1_bss.function_stack_check_reported || c1_bss.parser_early_result || c1_bss.function_entry_label || c1_bss.function_expression_offset || c1_bss.function_symbol_offset || c1_bss.function_output_mode || c1_bss.parameter_symbol_count || c1_bss.lexical_skip_mode || c1_bss.parameter_scope_value || c1_bss.function_storage_class || c1_bss.function_reset_value || c1_bss.nonil_type_index || c1_bss.parameter_storage_class || c1_bss.parameter_scope_initial_value || c1_bss.unevaluated_expression_depth || c1_bss.parameter_nesting ||
        c1_bss.declaration_control[1].flags || c1_bss.declaration_depth || c1_bss.declaration_control[0].flags ||
        c1_bss.pending_scope_symbols || c1_data.try_depth || c1_data.finally_depth || c1_data.exception_filter_depth ||
        c1_bss.type_description_depth || c1_bss.type_descriptions[1].flags ||
        c1_bss.type_descriptions[1].attributes || c1_bss.type_descriptions[1].shape ||
        c1_bss.type_descriptions[1].auxiliary || c1_bss.type_descriptions[1].debug_index)
        return 0;
    return c1_data.browser_last_line == 239 && saved->browser_last_line == 239 &&
        c1_data.browser_absolute_line == 17 && saved->browser_absolute_line == 17 &&
        c1_data.browser_output_stream == (FILE *)&owned_symbols[0] && saved->browser_output_stream == (FILE *)&owned_symbols[0] &&
        c1_data.browser_suppressed == 67 && saved->browser_suppressed == 67 &&
        c1_data.browser_header_position == 113 && saved->browser_header_position == 113 &&
        c1_data.browser_generation == 137 && saved->browser_generation == 137 &&
        c1_data.function_directory_head == (struct CompilerFunctionDirectoryBlock *)&owned_symbols[0] &&
        c1_data.function_directory_current == (struct CompilerFunctionDirectoryBlock *)&owned_symbols[1] &&
        c1_data.function_directory_index == 7 && c1_data.function_debug_handle == 37 && c1_data.function_cleanup_flags == 71 && c1_data.debug_symbol_index == 79 && c1_data.function_scope_cache == &owned_symbols[1] && c1_data.source_exhausted == 17 && c1_data.final_input_token_emitted == 29 && c1_data.omit_external_names == 37 && c1_data.pch_type_record_value == 0x1234 && c1_data.pch_record_name == (const char *)&owned_symbols[0] && c1_data.global_options == (struct CompilerGlobalOption *)&owned_symbols[1] && c1_data.pch_expression_stream_open == 17 && c1_data.pch_primary_stream_open == 19 && c1_data.pch_secondary_stream_open == 23 && c1_data.pch_cleanup_done == 29 && c1_data.saved_context_records == (struct CompilerSavedContextRecord *)&owned_symbols[0] && c1_data.saved_context_count == 17 &&
        c1_data.label_symbols == &owned_symbols[0] && saved->label_symbols == &owned_symbols[0] &&
        c1_data.line_range_warning_enabled == 37 && saved->line_range_warning_enabled == 37 &&
        c1_data.function_record_flags == 0x12345678 && saved->function_record_flags == 0x12345678 &&
        c1_data.function_label_state_first == 37 && saved->function_label_state_first == 37 &&
        c1_data.function_label_state_second == 43 && saved->function_label_state_second == 43 &&
        c1_data.parser_flags == 0x1234 && saved->parser_flags == 0x1234 &&
        c1_data.function_return_type == (CompilerType *)&owned_symbols[0] &&
        saved->function_return_type == (CompilerType *)&owned_symbols[0] &&
        c1_data.function_label_depth == 17 && saved->function_label_depth == 17 &&
        c1_data.local_symbol_limit == 0x12345 && saved->local_symbol_limit == 0x12345 &&
        c1_data.function_reference_epoch == 37 && saved->function_reference_epoch == 37 &&
        c1_data.disable_inline_records == 43 && saved->disable_inline_records == 43 &&
        c1_data.function_context == &owned_symbols[0] && saved->function_context == &owned_symbols[0] && c1_data.lexer_flags == 0xa537U && saved->lexer_flags == 0xa537U &&
        c1_data.active_parser == (struct CompilerParser *)&owned_symbols[0] &&
        saved->active_parser == (struct CompilerParser *)&owned_symbols[0] &&
        !c1_bss.parser_value.declaration_flags && c1_bss.browser_context_value == 0 && c1_data.debug_module_kind == 37 && saved->debug_module_kind == 37 &&
        c1_data.source_frames == &owned_frames[0] &&
        c1_data.root_source_frame == &owned_frames[1] &&
        c1_data.free_source_frames == &owned_frames[2] &&
        c1_data.source_descriptor == 41 && c1_data.conditional_depth == 13 &&
        c1_bss.source_cursor == NULL && c1_data.emit_global_locations == 37 && saved->emit_global_locations == 37 &&
        c1_data.source_include_depth == 13 && saved->source_include_depth == 13 &&
        c1_data.macro_reset_value == 83 && saved->macro_reset_value == 83 &&
        c1_data.type_arena_kind == 2 && c1_bss.saved_data == saved &&
        c1_data.declaration_symbol_cache_index == 13 && saved->declaration_symbol_cache_index == 13 &&
        c1_data.member_symbol_cache_index == 37 && saved->member_symbol_cache_index == 37 &&
        c1_data.compiler_messages_initialized == 37 && saved->compiler_messages_initialized == 37 &&
        c1_data.compiler_messages[catalog_index].id == C1_TEXT_FUNCTION_THROUGH_POINTER &&
        c1_data.compiler_messages[catalog_index].text == (char *)&owned_symbols[0] &&
        saved->compiler_messages[catalog_index].text == (char *)&owned_symbols[0] &&
        c1_data.record_sizes.known.named_parameter_symbol_bytes == 41 && saved->record_sizes.known.named_parameter_symbol_bytes == 41 &&
        c1_data.record_sizes.known.declaration_symbol_bytes == 47 &&
        c1_data.record_sizes.known.bitfield_symbol_bytes == 49 && saved->record_sizes.known.bitfield_symbol_bytes == 49 &&
        saved->record_sizes.known.declaration_symbol_bytes == 47 &&
        c1_data.record_sizes.known.intrinsic_symbol_bytes == 37 &&
        c1_data.record_sizes.known.global_symbol_bytes == 41 &&
        c1_data.record_sizes.known.abstract_symbol_bytes == 43 &&
        c1_data.record_sizes.known.segment_symbol_bytes == 45 && saved->record_sizes.known.segment_symbol_bytes == 45 &&
        c1_data.inline_depth == 4 && c1_data.inline_recursion == 1 &&
        saved->inline_depth == 4 && saved->inline_recursion == 1 &&
        c1_data.pragma_restore_options == 0x12345678 && c1_data.pragma_disable_options == 0x87654321 &&
        saved->pragma_restore_options == 0x12345678 && saved->pragma_disable_options == 0x87654321 &&
        c1_data.anonymous_identifier_count == 53 && saved->anonymous_identifier_count == 53 &&
        c1_data.segment_symbol_cache_index == 11 && saved->segment_symbol_cache_index == 11 &&
        c1_data.declaration_context_map == (CompilerSymbolMap *)&owned_symbols[0] &&
        saved->declaration_context_map == (CompilerSymbolMap *)&owned_symbols[0] &&
        c1_data.global_symbol_cache_index == 17 &&
        c1_data.anonymous_global_symbol == &owned_symbols[0] &&
        c1_data.local_size_symbol == &owned_symbols[1] &&
        c1_data.intrinsic_symbol_cache_index == 7 &&
        c1_data.next_symbol_record_id == 83 &&
        !c1_reuse_state && !c1_bss.declaration_scope_active;
}
extern struct CompilerTypeShape *__cdecl make_type_shape(unsigned, unsigned, unsigned *);
static int type_shape_write_extent(void)
{
    DWORD storage[5];
    struct CompilerTypeShape *shape;
    unsigned i;
    for (i = 0; i < 5; ++i) storage[i] = 0xa5a5a5a5;
    c1_bss.arenas[3].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[3].committed_limit = (BYTE *)(storage + 4);
    shape = make_type_shape(1, 0, NULL);
    /* Owned sentinels check only the constructor's write extent. They are
     * never dereferenced or treated as original allocator defaults. */
    return (void *)shape == storage + 1 && shape->flags == 1 &&
        shape->attributes == 0 && (unsigned)shape->base == 0xa5a5a5a5 &&
        shape->value == 0xa5a5a5a5 && storage[0] == 0xa5a5a5a5 &&
        storage[4] == 0xa5a5a5a5 &&
        c1_bss.arenas[3].cursor == (BYTE *)(storage + 4);
}
extern int __cdecl intrinsic_enabled(unsigned);
static int intrinsic_selection_policy(void)
{
    c1_data.options.intrinsics = 0;
    if (!intrinsic_enabled(1) || intrinsic_enabled(0)) return 0;
    c1_data.options.intrinsics = 1;
    c1_data.options.language_extensions = 0;
    c1_data.options.precise_float = 0;
    if (!intrinsic_enabled(0) || intrinsic_enabled(40)) return 0;
    c1_data.options.language_extensions = 1;
    if (!intrinsic_enabled(40)) return 0;
    c1_data.options.precise_float = 1;
    return !intrinsic_enabled(40) && intrinsic_enabled(31) && intrinsic_enabled(1) &&
        c1_data.options.intrinsics == 1 && c1_data.options.language_extensions == 1 &&
        c1_data.options.precise_float == 1;
}
extern int c1_test_arena_byte_allocation(void);
extern void __cdecl initialize_intrinsic_symbol(CompilerIrSymbol *, unsigned);
static int intrinsic_record_fields(void)
{
    DWORD storage[15];
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)(storage + 1);
    BYTE *bytes = (BYTE *)symbol;
    unsigned i;
    for (i = 0; i < 15; ++i) storage[i] = 0xa5a5a5a5;
    initialize_intrinsic_symbol(symbol, 1);
    if (symbol->kind != 6 || symbol->intrinsic_flags != 1 ||
        symbol->intrinsic_opcode != 0xdb || symbol->intrinsic_traits != 0x81)
        return 0;
    for (i = 0; i < 52; ++i)
        if (i != 0 && (i < 28 || i >= 36) && bytes[i] != 0xa5) return 0;
    return storage[0] == 0xa5a5a5a5 && storage[14] == 0xa5a5a5a5;
}
extern int c1_test_scope_binding_chains(void);
extern struct CompilerTypeShape *__cdecl copy_type_shape(const struct CompilerTypeShape *);
static int shallow_type_shape_copy(void)
{
    DWORD storage[5];
    struct CompilerTypeShape input, retained, *copy;
    unsigned i;
    for (i = 0; i < 5; ++i) storage[i] = 0xa5a5a5a5;
    input.flags = 0x1234;
    input.attributes = 0x5678;
    input.base = &retained;
    input.value = 37;
    c1_bss.arenas[3].cursor = (BYTE *)(storage + 1);
    c1_bss.arenas[3].committed_limit = (BYTE *)(storage + 4);
    copy = copy_type_shape(&input);
    return (void *)copy == storage + 1 && copy != &input &&
        copy->flags == 0x1234 && copy->attributes == 0x5678 &&
        copy->base == &retained && copy->value == 37 &&
        input.flags == 0x1234 && input.attributes == 0x5678 &&
        input.base == &retained && input.value == 37 &&
        c1_bss.arenas[3].cursor == (BYTE *)(storage + 4) &&
        storage[0] == 0xa5a5a5a5 && storage[4] == 0xa5a5a5a5;
}
extern void *__cdecl allocate_symbol_record(unsigned, unsigned, unsigned);
static int symbol_size_indexing(void)
{
    BYTE owned[130];
    unsigned i;
    memset(owned, 0xa5, sizeof(owned));
    c1_data.record_sizes.known.intrinsic_symbol_bytes = 37;
    c1_data.record_sizes.known.global_symbol_bytes = 41;
    c1_data.record_sizes.known.abstract_symbol_bytes = 43;
    c1_bss.arenas[1].cursor = owned + 1;
    c1_bss.arenas[1].committed_limit = owned + 129;
    if (allocate_symbol_record(999, 0x106, 1) != owned + 1 ||
        c1_bss.arenas[1].cursor != owned + 41 ||
        allocate_symbol_record(1, 9, 1) != owned + 41 ||
        c1_bss.arenas[1].cursor != owned + 85 ||
        allocate_symbol_record(0, 13, 1) != owned + 85 ||
        c1_bss.arenas[1].cursor != owned + 129) return 0;
    for (i = 0; i < sizeof(owned); ++i)
        if (owned[i] != 0xa5) return 0;
    return 1;
}
extern void __cdecl initialize_macro_buffers(void);
static int macro_buffer_reset(void)
{
    BYTE first[3] = { 0xa5, 0x96, 0x87 };
    BYTE second[3] = { 0x78, 0x69, 0x5a };
    c1_data.macro_reset_value = 37;
    c1_bss.work_buffers[0].data = first + 1;
    c1_bss.work_buffers[1].data = second + 1;
    c1_bss.macro_token_cursor = second + 2;
    c1_bss.macro_text_cursor = first;
    c1_bss.saved_macro_text_cursor = first + 2;
    initialize_macro_buffers();
    return !c1_data.macro_reset_value &&
        c1_bss.macro_token_cursor == first + 1 &&
        c1_bss.macro_text_cursor == second + 1 &&
        c1_bss.saved_macro_text_cursor == second + 1 &&
        first[0] == 0xa5 && first[1] == 0x96 && first[2] == 0x87 &&
        second[0] == 0x78 && second[1] == 0x69 && second[2] == 0x5a;
}
extern int c1_test_macro_definition_extent(void);
extern void __cdecl write_global_record(unsigned, struct CompilerIrSymbol *);
static int global_header_record(void)
{
    BYTE bytes[40];
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[2];
    unsigned i;
    static const BYTE expected[12] = {
        17, 0x86, 3, 0x60, 0x92, 0x30, 1, 0x78, 0x56, 0x34, 0x12, 0
    };
    memset(bytes, 0xa5, sizeof(bytes));
    memset(buffer, 0, sizeof(*buffer));
    buffer->data = bytes + 4;
    buffer->cursor = bytes + 4;
    buffer->capacity = 32;
    buffer->good = 1;
    c1_data.c9_il = 1;
    c1_data.report_statistics = 0;
    c1_data.emit_global_locations = 37; /* Only opcode14 consumes the flag. */
    c1_data.next_symbol_record_id = 0x12345678;
    c1_bss.declaration_scope_active = 0;
    write_global_record(17, NULL);
    if (buffer->cursor != bytes + 16 || !buffer->good ||
        memcmp(bytes + 4, expected, sizeof(expected)) ||
        c1_data.next_symbol_record_id != 0x12345678 ||
        c1_data.emit_global_locations != 37) return 0;
    for (i = 0; i < sizeof(bytes); ++i)
        if ((i < 4 || i >= 16) && bytes[i] != 0xa5) return 0;
    /* The final NULL record has a scalar chain value and two absent lists. */
    c1_data.pch_type_record_value = 0x1234;
    c1_data.pch_record_name = NULL;
    c1_data.global_options = NULL;
    buffer->cursor = bytes + 4;
    memset(bytes, 0xa5, sizeof(bytes));
    write_global_record(10, NULL);
    if (buffer->cursor != bytes + 12 || bytes[4] != 10 || bytes[5] != 0x80 ||
        *(DWORD *)(bytes + 6) != 0x1234 || bytes[10] || bytes[11] ||
        c1_data.pch_type_record_value != 0x1234) return 0;
    for (i = 0; i < sizeof(bytes); ++i)
        if ((i < 4 || i >= 12) && bytes[i] != 0xa5) return 0;
    return 1;
}
extern int c1_test_name_span_interner(void);
extern int c1_test_source_file_write_extent(void);
extern int c1_test_context_buffer_initialize(void);
extern int c1_test_context_push_alias(void);
extern CompilerType *__cdecl copy_type_description(const CompilerType *);
extern CompilerType *__cdecl get_function_return_type(CompilerType *);
static int type_description_storage_copy(void)
{
    CompilerType source, before;
    unsigned char neighbors[sizeof(CompilerType)];
    memset(&source, 0x96, sizeof(source));
    source.flags = 0x63;
    source.attributes = 0x100;
    source.shape = NULL;
    source.auxiliary = 17;
    /* The last DWORD is opaque for this copy: preserve its byte pattern. */
    before = source;
    memset(&c1_bss.type_descriptions[0], 0xa5,
           sizeof(c1_bss.type_descriptions));
    memcpy(neighbors, &c1_bss.type_descriptions[0], sizeof(neighbors));
    c1_bss.type_description_depth = 0;
    if (copy_type_description(&source) != &c1_bss.type_descriptions[1] ||
        c1_bss.type_description_depth != 1 || memcmp(&source, &before, sizeof(source)) ||
        memcmp(&c1_bss.type_descriptions[1], &source, sizeof(source)) ||
        memcmp(&c1_bss.type_descriptions[0], neighbors, sizeof(neighbors))) return 0;
    {
        struct CompilerTypeShape function, remainder;
        DWORD storage[6];
        CompilerType expected;
        memset(storage, 0xa5, sizeof(storage));
        memset(&function, 0x37, sizeof(function));
        function.base = &remainder;
        source.shape = &function;
        before = source;
        expected = source;
        expected.shape = &remainder;
        c1_data.type_arena_kind = 2;
        c1_bss.arenas[2].cursor = (BYTE *)(storage + 1);
        c1_bss.arenas[2].committed_limit = (BYTE *)(storage + 5);
        if (get_function_return_type(&source) != (CompilerType *)(storage + 1) ||
            c1_bss.arenas[2].cursor != (BYTE *)(storage + 5) ||
            memcmp(storage + 1, &expected, 16) || memcmp(&source, &before, 16) ||
            storage[0] != 0xa5a5a5a5 || storage[5] != 0xa5a5a5a5) return 0;
    }
    return 1;
}
extern void __cdecl write_ir_operand(int, const CompilerIntegerConstant *, int);
extern void __cdecl write_ir_integer_words(unsigned, unsigned, int);
static int scalar_ir_encodings(void)
{
    static const BYTE expected[] = {
        0xa0, 0x41, 0x7f, 0x81,
        0x80, 0x80, 0xff, 0xff, 0xff,
        0x80, 0x80, 0, 0, 0,
        0x80, 0, 0, 0, 0, 1, 0, 0, 0
    };
    BYTE bytes[64];
    CompilerType type;
    CompilerIntegerConstant node;
    CompilerOutputBuffer *buffer = &c1_bss.output_buffers[3];
    unsigned i;
    memset(bytes, 0xa5, sizeof(bytes));
    memset(&type, 0x96, sizeof(type));
    memset(&node, 0x87, sizeof(node));
    type.flags = 0x63; type.attributes = 0x100; type.shape = NULL;
    node.type = &type;
    memset(buffer, 0, sizeof(*buffer));
    buffer->data = buffer->cursor = bytes + 4;
    buffer->capacity = 56; buffer->good = 7;
    c1_bss.parser_early_result = 0;
    write_ir_operand(51, &node, 1);
    write_ir_integer_words(127, 0, 4);
    write_ir_integer_words(0xffffff81, 0xffffffff, 4);
    write_ir_integer_words(0xffffff80, 0xffffffff, 4);
    write_ir_integer_words(128, 0, 4);
    write_ir_integer_words(0, 1, 8);
    if (buffer->cursor != bytes + 4 + sizeof(expected) || buffer->good != 1 ||
        memcmp(bytes + 4, expected, sizeof(expected))) return 0;
    for (i = 0; i < sizeof(bytes); ++i)
        if ((i < 4 || i >= 4 + sizeof(expected)) && bytes[i] != 0xa5) return 0;
    return 1;
}
extern int c1_test_arena_clear_used(void);
extern int c1_test_function_directory(void);
extern int c1_test_recorded_trace(void);
extern int c1_test_location_token(void);
static int pdb_context_initializers(void)
{
    CompilerPdbContext *context;
    unsigned i, j;
    c1_reuse_state = 0;
    memset(&c1_pdb_context, 0xa5, sizeof(c1_pdb_context));
    memset(&c1_output_pdb_context, 0x5a, sizeof(c1_output_pdb_context));
    if (initialize_pdb_context_12676() || initialize_pdb_context_126b6()) return 0;
    for (i = 0; i < 2; ++i) {
        context = i ? &c1_output_pdb_context : &c1_pdb_context;
        if (context->database || context->names || context->types ||
            context->retain_database || context->rebuild_state || context->read_mode) return 0;
        for (j = 0; j < 4; ++j)
            if (context->unmodeled_10[j] != (i ? 0x5a : 0xa5)) return 0;
    }
    c1_reuse_state = 1;
    c1_pdb_context.read_mode = 17;
    c1_output_pdb_context.retain_database = 23;
    return !initialize_pdb_context_12676() && !initialize_pdb_context_126b6() &&
        c1_pdb_context.read_mode == 17 && c1_output_pdb_context.retain_database == 23;
}
extern int c1_test_output_destroy(void);
const ReconTest minimal_tests[60] = {
    { "output destruction closes owned resources and retains unmodeled storage", 1, c1_test_output_destroy },
    { "PDB contexts clear six known fields and retain the unmodeled DWORD", 1, pdb_context_initializers },
    { "recorded token blocks retain gaps and chain at twenty seven tokens", 1, c1_test_recorded_trace },
    { "location token retains opaque storage and writes only context and kind", 1, c1_test_location_token },
    { "function directory records retain flags and encode function indexes", 1, c1_test_function_directory },
    { "IR scalar type and signed integer encodings retain boundaries and owned tails", 1, scalar_ir_encodings },
    { "arena clear zeros exactly the used span and retains committed storage and neighbors", 1, c1_test_arena_clear_used },
    { "type description copies sixteen storage bytes including opaque cache and retains neighbor", 1, type_description_storage_copy },
    { "based literal type selection retains signed, unsigned and configured-width boundaries", 1, c1_test_based_literal_boundaries },
    { "block scope owns24 bytes and preserves bucket pointer and padding", 1, c1_test_block_scope_extent },
    { "parameter and hash scope layouts and shared slot", 1, c1_test_parameter_scope_layout },
    { "context push captures aliased source before publishing lexer state", 1, c1_test_context_push_alias },
    { "context buffer owns128 bytes and initializes only its first link", 1, c1_test_context_buffer_initialize },
    { "source-file allocation preserves unwritten ID, gaps and owned neighbors", 1, c1_test_source_file_write_extent },
    { "name span interning honors supplied length, collision order and creation result", 1, c1_test_name_span_interner },
    { "global header emits twelve defined bytes and retains owned neighbors", 1, global_header_record },
    { "macro definition owns descriptor and length-defined writes only", 1, c1_test_macro_definition_extent },
    { "macro reset preserves work-buffer identities and all owned bytes", 1, macro_buffer_reset },
    { "symbol allocator uses only known indexed sizes, byte kind and aligned stride", 1, symbol_size_indexing },
    { "shape interning keeps collision keys, canonical identity and16-byte ownership", 1, interned_type_shapes },
    { "parser constructor writes only known header fields in owned8864 bytes", 1, c1_test_parser_reset_extent },
    { "option bits preserve sparse positions and boolean consumption", 1, packed_option_bits },
    { "null debug module preserves all 58 physical method identities", 1, c1_test_null_debug_factory },
    { "incremental path suffix writes exactly four bytes and retains tails", 1, incremental_path_extent },
    { "primitive width encodings and literal-width alternative", 1, primitive_width_relation },
    { "type allocation arena default and nonzero snapshot restore", 1, type_arena_snapshot },
    { "pointer shape writes only declared fields within its12-byte allocation", 1, type_shape_write_extent },
    { "intrinsic selection retains mandatory names and respects extension and floating policy", 1, intrinsic_selection_policy },
    { "byte arena allocation preserves exact stride and owned bytes at the limit", 1, c1_test_arena_byte_allocation },
    { "intrinsic metadata retains symbol payload and writes only known classification fields", 1, intrinsic_record_fields },
    { "scope collisions link embedded bindings and retain symbol payload", 1, c1_test_scope_binding_chains },
    { "type shape copy is shallow, distinct and confined to12 owned bytes", 1, shallow_type_shape_copy },
    { "packing alignment inheritance and explicit override", 1, packing_alignment },
    { "input factory interfaces and recorded-buffer defaults", 1, c1_test_input_factory },
    { "processor mask replaces prior value and adds compatibility bit", 1, processor_instruction_mask },
    { "browser factory shared slots and owned filename prefix", 1, c1_test_browser_factory },
    { "backend getter preserves existing identity and lazily constructs", 1, c1_test_backend_getter },
    { "fixed-token prototype tags, stride and untouched bytes", 1, predefined_token_prototypes },
    { "keyword storage extent, lazy identity and untouched bytes", 1, keyword_storage },
    { "identifier collision, move-to-front, retained flags and allocation stride", 1, c1_test_identifier_interning },
    { "output constructor clears only first filename byte", 1, output_filename_constructor },
    { "integer node signed payload, type identity, zero flag and24-byte stride", 1, integer_nodes },
    { "line delta encoding, duplicate suppression and zero retention", 1, pending_line_record },
    { "binary node construction writes only known fields in its24-byte owner", 1, c1_test_binary_node_extent },
    { "signed short encoding preserves negative and wide boundaries within owned bytes", 1, c1_test_output_short },
    { "index encoding retains signed boundary, exact bytes and failed-buffer state", 1, c1_test_output_index },
    { "small operand encoding rejects wide values without writes and retains neighbors", 1, c1_test_output_small },
    { "output reservation bounds and no state mutation", 1, c1_test_output_ensure },
    { "buffer growth preserves used bytes, zeros tail and publishes relocation", 1, c1_test_output_grow },
    { "flush suppression resets cursor and retains bytes without stream access", 1, c1_test_output_flush_suppression },
    { "type hash tables own exact arena spans without clearing them", 1, type_table_storage },
    { "type interning shares semantic keys, retains collision chains and handles debug cache", 1, type_interning },
    { "global symbol arena replaces saved context without reading it", 1, global_symbol_arena },
    { "returning interner uses ordinary-name stride and retains untouched bytes", 1, c1_test_returning_interner },
    { "translation context constructs root scope and preserves unwritten slots", 1, c1_test_translation_unit },
    { "abstract symbol cache reuses only head9 within its owned52-byte record", 1, c1_test_abstract_symbol_reuse },
    { "symbol constructor retains untouched fields and clears kind4 context only", 1, c1_test_symbol_constructor },
    { "type shape allocation truncates WORD fields and retains unwritten link", 1, type_shape_allocation },
    { "empty parameter list is lazy, shared and leaves unused element untouched", 1, cached_empty_type_list },
    { "shape attachment preserves empty context and folds prior pointer kind", 1, c1_test_append_type_shape }
};
