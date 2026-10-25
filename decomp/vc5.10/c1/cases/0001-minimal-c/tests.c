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
    initialize_output_buffer_124c1();
    if (buffer->filename[0] || buffer->file_descriptor != -1) return 0;
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
        node->flags != 3 || node->type != &type || node->integer != -17) return 0;
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
const ReconTest minimal_tests[22] = {
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
    { "type shape allocation truncates WORD fields and retains unwritten link", 1, type_shape_allocation }
};
