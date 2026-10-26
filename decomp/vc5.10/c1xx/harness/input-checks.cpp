/* Native-only layout/alias check; no target virtual method is invoked. */
#include <string.h>
#include <new.h>
#include "frontend-init.h"
#include "lexer.h"
#include "parser-actions.h"
#include "parser.h"
#include <recon.h>

extern "C" int create_input(int, CompilerInput **, CompilerTextInput **);
extern "C" void canonical_reset(void) __asm__("_ZN13CompilerLexer5ResetEv");

extern "C" int c1xx_test_input_alias(void)
{
    CompilerInput *input;
    CompilerTextInput *text;
    void **table;
    if (create_input(0, &input, &text) != 1 || !input || !text)
        return 0;
    if ((BYTE *)text != (BYTE *)input + 4)
        return 0;
    table = *(void ***)text;
    return table[2] == (void *)canonical_reset;
}

extern "C" void begin_browser_output(const char *);
extern "C" void canonical_trace(unsigned) __asm__("_ZN13CompilerLexer5TraceEj");
extern "C" int c1xx_test_browser_aliases(void)
{
    static const unsigned reset_slots[] = {0,1,2,3,4,6,11,13,14,16,18};
    static const unsigned trace_slots[] = {5,7,9,12,15,17,19,20};
    void **table;
    unsigned i;
    begin_browser_output(NULL);
    if (!frontend_data.initial_backend || sizeof(CompilerBackend) != 4)
        return 0;
    table = *(void ***)frontend_data.initial_backend;
    for (i = 0; i < sizeof(reset_slots)/sizeof(reset_slots[0]); ++i)
        if (table[reset_slots[i]] != (void *)canonical_reset) return 0;
    for (i = 0; i < sizeof(trace_slots)/sizeof(trace_slots[0]); ++i)
        if (table[trace_slots[i]] != (void *)canonical_trace) return 0;
    return 1;
}

/* Check the actual C++ table without invoking pending virtual methods. */
extern "C" void scope_has_buckets(void) __asm__("_ZN13CompilerScope10HasBucketsEv");
extern "C" void scope_has_secondary(void) __asm__("_ZN13CompilerScope12HasSecondaryEv");
extern "C" void scope_insert(void) __asm__("_ZN13CompilerScope6InsertEP14CompilerSymbol");
extern "C" void scope_find(void) __asm__("_ZN13CompilerScope4FindEP17CompilerNameEntryj");
extern "C" void scope_find_ordinary(void) __asm__("_ZN13CompilerScope12FindOrdinaryEP17CompilerNameEntry");
extern "C" void scope_lookup(void) __asm__("_ZN13CompilerScope6LookupEP17CompilerNameEntryjPj");
extern "C" void scope_find_nested(void) __asm__("_ZN13CompilerScope10FindNestedEP17CompilerNameEntryPP14CompilerSymbol");
extern "C" void scope_find_link(void) __asm__("_ZN13CompilerScope8FindLinkEP18CompilerSymbolLink");
extern "C" void scope_serialize(void) __asm__("_ZN13CompilerScope9SerializeEv");
extern "C" void scope_check(void) __asm__("_ZN13CompilerScope5CheckEv");
extern "C" void scope_link(void) __asm__("_ZN13CompilerScope4LinkEP14CompilerSymbol");
extern "C" int c1xx_test_scope_table(void)
{
    CompilerScope scope;
    void **table = *(void ***)&scope;
    static void (*const expected[])(void) = {scope_has_buckets, scope_has_secondary, scope_insert, scope_find, scope_find_ordinary, scope_lookup, scope_find_nested, scope_find_link, scope_serialize, scope_check, scope_link};
    unsigned i;
    for (i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i)
        if (table[i] != (void *)expected[i]) return 0;
    return sizeof(scope) == 32 && scope.next == NULL && scope.level == 0 &&
        scope.outer == NULL && scope.context == NULL && scope.bucket_mask == 0x3ff &&
        scope.secondary == NULL && sizeof(CompilerNamespaceSymbol) <= 20;
}

extern "C" CompilerSymbol *recycle_symbol(unsigned, CompilerNameEntry *, unsigned);
extern "C" int c1xx_test_recycled_symbol(void)
{
    union { DWORD alignment[5]; BYTE bytes[20]; } storage;
    CompilerNamespaceSymbol *symbol = (CompilerNamespaceSymbol *)storage.bytes;
    CompilerNameEntry name;
    CompilerSymbolLink tail;
    unsigned i;
    for (i = 0; i < sizeof(storage.bytes); ++i) storage.bytes[i] = 0xa5;
    symbol->link.name = NULL; symbol->link.next = &tail;
    frontend_data.recycled_symbols[6] = &symbol->link;
    if (recycle_symbol(0x117, &name, 1) != NULL ||
        frontend_data.recycled_symbols[6] != &symbol->link) return 0;
    if (recycle_symbol(0x117, &name, 0) != symbol ||
        frontend_data.recycled_symbols[6] != &tail) return 0;
    /* Test the known semantic fields and the actual owned zero-write span;
     * no meaning is assigned to the original record's unrecovered bytes. */
    if (symbol->kind != 0x17 || symbol->attributes || symbol->definition_flags ||
        symbol->link.name != &name || symbol->link.next || symbol->data) return 0;
    for (i = sizeof(*symbol); i < sizeof(storage.bytes); ++i)
        if (storage.bytes[i]) return 0;
    return 1;
}

extern "C" int create_debug_module(PDB *, int, NameMap *, const char *, const char *, CompilerTextInput *, int, int, int, CompilerDebugModule **);
extern "C" void debug_table_target_0(void) __asm__("_ZN13CompilerLexer5ResetEv");
extern "C" void debug_table_target_1(void) __asm__("_ZN13CompilerLexer5TraceEj");
extern "C" void debug_table_target_2(void) __asm__("_ZN13CompilerLexer5ValidEv");
extern "C" void debug_table_target_3(void) __asm__("_ZN19CompilerNullBrowser6Slot54Ejj");
extern "C" void debug_table_target_4(void) __asm__("_ZN23CompilerNullDebugModule5CloseEv");
extern "C" void debug_table_target_5(void) __asm__("_ZN23CompilerNullDebugModule6Slot08EPj");
extern "C" void debug_table_target_6(void) __asm__("_ZN23CompilerNullDebugModule6Slot14EjjjPjS0_S0_S0_");
extern "C" void debug_table_target_7(void) __asm__("_ZN23CompilerNullDebugModule6Slot18EjjjPjS0_");
extern "C" void debug_table_target_8(void) __asm__("_ZN23CompilerNullDebugModule6Slot20Ej");
extern "C" void debug_table_target_9(void) __asm__("_ZN23CompilerNullDebugModule6Slot3cEjjPj");
extern "C" void debug_table_target_10(void) __asm__("_ZN23CompilerNullDebugModule6Slot54EjPi");
extern "C" void debug_table_target_11(void) __asm__("_ZN23CompilerNullDebugModule6Slot7cEv");
extern "C" void debug_table_target_12(void) __asm__("_ZN23CompilerNullDebugModule6Slotc0Ejjj");
extern "C" void debug_table_target_13(void) __asm__("_ZN23CompilerNullDebugModule6SlotccEjPj");
extern "C" int c1xx_test_debug_table(void)
{
    CompilerDebugModule *module;
    unsigned i;
    void **table;
    static void (*const expected[])(void) = {debug_table_target_2, debug_table_target_4, debug_table_target_5, debug_table_target_5, debug_table_target_2, debug_table_target_6, debug_table_target_7, debug_table_target_2, debug_table_target_8, debug_table_target_8, debug_table_target_2, debug_table_target_2, debug_table_target_3, debug_table_target_8, debug_table_target_3, debug_table_target_9, debug_table_target_2, debug_table_target_2, debug_table_target_2, debug_table_target_2, debug_table_target_2, debug_table_target_10, debug_table_target_5, debug_table_target_5, debug_table_target_0, debug_table_target_2, debug_table_target_2, debug_table_target_2, debug_table_target_0, debug_table_target_8, debug_table_target_8, debug_table_target_11, debug_table_target_2, debug_table_target_11, debug_table_target_8, debug_table_target_3, debug_table_target_9, debug_table_target_3, debug_table_target_2, debug_table_target_3, debug_table_target_3, debug_table_target_2, debug_table_target_3, debug_table_target_3, debug_table_target_2, debug_table_target_2, debug_table_target_2, debug_table_target_2, debug_table_target_12, debug_table_target_3, debug_table_target_11, debug_table_target_13, debug_table_target_2, debug_table_target_2, debug_table_target_1, debug_table_target_11, debug_table_target_3, debug_table_target_2};
    if (sizeof(CompilerNullDebugModule) != 4 ||
        !create_debug_module(NULL, 0, NULL, NULL, NULL, NULL, 0, 0, 0, &module) || !module)
        return 0;
    table = *(void ***)module;
    for (i = 0; i < 58; ++i)
        if (table[i] != (void *)expected[i]) return 0;
    return 1;
}


/* Read the actual compiler-emitted table; do not invoke pending methods or
 * construct an unvisited recorded-input owner. Two slots are code-free
 * aliases of pre-existing canonical physical functions. */
extern "C" void *const recorded_lexer_vtable[] __asm__("_ZTV21CompilerRecordedLexer");
extern "C" void recorded_read(void) __asm__("_ZN21CompilerRecordedLexer4ReadEv");
extern "C" void recorded_peek(void) __asm__("_ZN21CompilerRecordedLexer4PeekEv");
extern "C" void recorded_unread(void) __asm__("_ZN21CompilerRecordedLexer6UnreadEj");
extern "C" void recorded_trace(void) __asm__("_ZN21CompilerRecordedLexer5TraceEj");
extern "C" void recorded_valid_canonical(void) __asm__("_ZN23CompilerNullDebugModule6Slot7cEv");
extern "C" void recorded_reset(void) __asm__("_ZN21CompilerRecordedLexer5ResetEv");
extern "C" void recorded_destroy_canonical(void) __asm__("_ZN13CompilerLexer7DestroyEj");
extern "C" int c1xx_test_recorded_lexer_table(void)
{
    static void (*const expected[])(void) = {
        recorded_read, recorded_peek, recorded_unread, recorded_trace,
        recorded_valid_canonical, recorded_reset, recorded_destroy_canonical
    };
    unsigned i;
    for (i = 0; i < 7; ++i)
        if (recorded_lexer_vtable[i + 2] != (void *)expected[i]) return 0;
    return sizeof(CompilerRecordedLexer) == 64;
}

/* Actual scanner copy must retain the unknown representation bytes too. */
extern "C" ParserTokenView *__cdecl scan_numeric_token(unsigned, int);
extern "C" int c1xx_test_literal_storage(void)
{
    CompilerType type;
    TokenValue expected;
    union { __int64 alignment; BYTE bytes[48]; } storage;
    BYTE text[] = "2 ";
    unsigned i;
    memset(&type, 0, sizeof(type));
    type.flags = 0x63;
    for (i = 0; i < 40; ++i)
        frontend_bss.saved_token_value.original_representation[i] = (BYTE)(i * 7 + 3);
    expected = frontend_bss.saved_token_value.known;
    expected.literal.type = &type;
    expected.literal.value = 42;
    frontend_data.compiler_flags = 0;
    frontend_data.option_zb = 4;
    frontend_data.builtin_types.type_0063 = &type;
    frontend_bss.options.preprocess_stdout = 0;
    frontend_bss.source_cursor = text;
    frontend_bss.symbol_arena_kind = 0;
    frontend_bss.pools[7].cursor = storage.bytes;
    frontend_bss.pools[7].remaining = 48;
    SentinelToken *token = (SentinelToken *)scan_numeric_token('4', 0);
    return sizeof(TokenValue) == 40 &&
        (BYTE *)&expected.literal.value - (BYTE *)&expected == 8 &&
        token == (SentinelToken *)storage.bytes && token->kind == 101 &&
        token->value.literal.type == &type && token->value.literal.value == 42 &&
        memcmp(&token->value, &expected, 40) == 0;
}

/* The existing24-byte integer constructor must leave both opaque gaps and
 * the following allocation untouched after restoring the real payload offset. */
extern "C" CompilerExpression *__cdecl allocate_integer_constant(int);
extern "C" int c1xx_test_expression_storage(void)
{
    union { __int64 alignment; BYTE bytes[64]; } storage;
    CompilerType type;
    CompilerArena previous = frontend_bss.arenas[0];
    unsigned previous_index = frontend_data.constant_arena_index;
    CompilerType *previous_type = frontend_data.builtin_types.type_0063;
    unsigned i;
    int ok;
    memset(storage.bytes, 0xd7, sizeof(storage.bytes));
    frontend_data.constant_arena_index = 0;
    frontend_data.builtin_types.type_0063 = &type;
    frontend_bss.arenas[0].cursor = storage.bytes;
    frontend_bss.arenas[0].committed_limit = storage.bytes + sizeof(storage.bytes);
    CompilerExpression *node = allocate_integer_constant(-7);
    ok = node == (CompilerExpression *)storage.bytes &&
        frontend_bss.arenas[0].cursor == storage.bytes + 24 &&
        node->kind == 0x65 && node->flags == 3 && node->type == &type &&
        node->value.integer == (__int64)-7;
    for (i = 2; i < 4; ++i) if (storage.bytes[i] != 0xd7) ok = 0;
    for (i = 8; i < 16; ++i) if (storage.bytes[i] != 0xd7) ok = 0;
    for (i = 24; i < sizeof(storage.bytes); ++i) if (storage.bytes[i] != 0xd7) ok = 0;
    frontend_bss.arenas[0] = previous;
    frontend_data.constant_arena_index = previous_index;
    frontend_data.builtin_types.type_0063 = previous_type;
    return ok;
}

extern "C" int c1xx_test_declaration_callback_owner(void)
{
    ParserSemanticView *saved = (ParserSemanticView *)&frontend_bss.saved_token_value;
    ParserDeclarationCallback callback = &CompilerTranslationUnit::InsertCurrentScope;
    struct { ParserDeclarationToken token; unsigned neighbor; } record;
    struct { ParserDeclarationReader reader; unsigned neighbor; } local;
    BYTE prefix[36];
    unsigned i;
    for (i = 0; i < 36; ++i) prefix[i] = (BYTE)(0x39 + i * 13);
    memcpy(&record.token.value, prefix, 36);
    record.neighbor = 0x914fa3;
    local.neighbor = 0x173429;
    record.token.value.callback = callback;
    local.reader.declaration_callback = record.token.value.callback;
    if (sizeof(record.token) != 52 || sizeof(local.reader) != 76 ||
        memcmp(&record.token.value, prefix, 36) ||
        record.neighbor != 0x914fa3 || local.neighbor != 0x173429 ||
        local.reader.declaration_callback != callback) return 0;
    struct { ParserDeclarationToken token; unsigned neighbor; } copied;
    memset(&copied, 0xa5, sizeof(copied));
    copied.neighbor = 0x19372a;
    ParserTokenView *result = ((ParserRecordedContext *)&copied.token)->Read(0x22,
        (const TokenValue *)&record.token.value);
    if (result != (ParserTokenView *)&copied.token || copied.token.kind != 0x22 ||
        memcmp(&copied.token.value, prefix, 36) || copied.token.value.callback != callback ||
        copied.neighbor != 0x19372a || memcmp(&record.token.value, prefix, 36) ||
        record.token.value.callback != callback || record.neighbor != 0x914fa3) return 0;
    for (i = 1; i < 8; ++i)
        if (((BYTE *)&copied.token)[i] != 0xa5) return 0;
    CompilerType type;
    memset(&type, 0, sizeof(type));
    type.attributes = 0x1937;
    record.token.value.type = &type;
    record.token.value.object = (CompilerSymbol *)&frontend_data;
    memset(&local.reader, 0xa5, sizeof(local.reader));
    local.reader.token = (ParserTokenView *)&record.token;
    local.reader.CopyClassHead();
    if (local.reader.copied_class_head != 1 || local.reader.type != &type ||
        local.reader.type_symbol != record.token.value.object ||
        local.reader.declaration_callback != callback || local.reader.qualifier_flags != 0x1937 ||
        local.reader.type_flags != 0xa5a5 || local.reader.declaration_marked != 0xa5a5a5a5 ||
        memcmp(&local.reader.context, &record.token.value.context, 28) ||
        local.neighbor != 0x173429 || record.neighbor != 0x914fa3) return 0;
    for (i = 4; i < 20; ++i)
        if (((BYTE *)&local.reader)[i] != 0xa5) return 0;
    unsigned next_cell = 0x71938a4;
    frontend_bss.tag_novtordisp = next_cell;
    saved->declaration.callback = callback;
    return sizeof(CompilerSavedTokenValue) == 44 && sizeof(ParserSemanticView) == 44 &&
        sizeof(TokenValue) == 40 && sizeof(callback) == 8 &&
        saved->declaration.callback == callback &&
        frontend_bss.tag_novtordisp == next_cell;
}

extern "C" int c1xx_test_parser_semantic_slots(void)
{
    CompilerParser parser;
    ParserDeclarationToken token;
    ParserDeclarationCallback callback = &CompilerTranslationUnit::InsertCurrentScope;
    unsigned i;
    memset(&parser, 0xa5, sizeof(parser));
    for (i = 0; i < 36; ++i)
        ((BYTE *)&token.value)[i] = (BYTE)(0x31 + i * 17);
    token.kind = 0x22;
    token.value.callback = callback;
    copy_token_value(&parser.values[1023], (ParserTokenView *)&token);
    if (sizeof(parser) != 49164 || sizeof(parser.values[0]) != 44 ||
        (BYTE *)&parser.values[1] - (BYTE *)&parser.values[0] != 44 ||
        memcmp(&parser.values[1023], &token.value, 36) ||
        parser.values[1023].declaration.callback != callback ||
        parser.states[0] != 0xa5a5a5a5) return 0;
    for (i = 0; i < sizeof(parser.values[1022]); ++i)
        if (((BYTE *)&parser.values[1022])[i] != 0xa5) return 0;
    SentinelToken separator;
    separator.kind = 117;
    for (i = 0; i < 40; ++i)
        separator.value.original_representation[i] = (BYTE)(0x91 + i * 3);
    copy_token_value(&parser.values[1022], (ParserTokenView *)&separator);
    if (memcmp(&parser.values[1022], &separator.value, 40)) return 0;
    for (i = 40; i < 44; ++i)
        if (((BYTE *)&parser.values[1022])[i] != 0xa5) return 0;
    return parser.values[1023].declaration.callback == callback;
}

extern "C" int c1xx_test_saved_lookup_state(void)
{
    CompilerTranslationUnit unit;
    CompilerSavedLookup saved, previous;
    CompilerScopeTable *scopes[2];
    CompilerScope first, second;
    CompilerSymbol qualified_owner;
    unsigned i;
    memset(&unit, 0xa5, sizeof(unit));
    memset(&saved, 0x73, sizeof(saved));
    unit.current = &first; unit.depth = 2;
    unit.retired_scopes = &qualified_owner; unit.lookup_context = 0x19;
    unit.suppress_lookup_errors = 0x1739;
    unit.opaque_lookup_state = 0x9127a319;
    unit.lookup_override = 0x1937;
    unit.scopes[1] = &first; unit.scopes[2] = &second;
    if (saved.Initialize(&unit, &previous, 0) != &saved || saved.scopes ||
        saved.current != &first || saved.depth != 2 || saved.retired != &qualified_owner ||
        saved.lookup_context != 0x19 || saved.suppress_lookup_errors != 0x1739 ||
        saved.opaque_lookup_state != 0x9127a319 || saved.lookup_override != 0x1937 ||
        saved.previous != &previous) return 0;
    for (i = 0; i < 3; ++i)
        if (saved.unmodeled_05_07[i] != 0x73 || saved.unmodeled_11_13[i] != 0x73)
            return 0;
    frontend_bss.arenas[1].cursor = (BYTE *)scopes;
    frontend_bss.arenas[1].committed_limit = (BYTE *)(scopes + 2);
    if (saved.Initialize(&unit, NULL, 1) != &saved || saved.scopes != scopes ||
        saved.previous || scopes[0] != &first || scopes[1] != &second ||
        frontend_bss.arenas[1].cursor != (BYTE *)(scopes + 2)) return 0;
    return sizeof(saved) == 36 && sizeof(unit) == 572 &&
        saved.opaque_lookup_state == 0x9127a319;
}

extern "C" void class_scope_slot_class_0(void) __asm__("_ZN18CompilerClassScope10HasBucketsEv");
extern "C" void class_scope_slot_class_1(void) __asm__("_ZN18CompilerClassScope12HasSecondaryEv");
extern "C" void class_scope_slot_class_2(void) __asm__("_ZN18CompilerClassScope6InsertEP14CompilerSymbol");
extern "C" void class_scope_slot_class_3(void) __asm__("_ZN18CompilerClassScope4FindEP17CompilerNameEntryj");
extern "C" void class_scope_slot_class_4(void) __asm__("_ZN18CompilerClassScope12FindOrdinaryEP17CompilerNameEntry");
extern "C" void class_scope_slot_class_5(void) __asm__("_ZN18CompilerClassScope6LookupEP17CompilerNameEntryjPj");
extern "C" void class_scope_slot_class_6(void) __asm__("_ZN18CompilerClassScope10FindNestedEP17CompilerNameEntryPP14CompilerSymbol");
extern "C" void class_scope_slot_class_7(void) __asm__("_ZN18CompilerClassScope8FindLinkEP18CompilerSymbolLink");
extern "C" void class_scope_slot_class_8(void) __asm__("_ZN18CompilerClassScope9SerializeEv");
extern "C" void class_scope_slot_class_9(void) __asm__("_ZN18CompilerClassScope5CheckEv");
extern "C" void class_scope_slot_class_10(void) __asm__("_ZN18CompilerClassScope4LinkEP14CompilerSymbol");
extern "C" void class_scope_slot_aux_0(void) __asm__("_ZN22CompilerAuxiliaryScope10HasBucketsEv");
extern "C" void class_scope_slot_aux_1(void) __asm__("_ZN22CompilerAuxiliaryScope12HasSecondaryEv");
extern "C" void class_scope_slot_aux_2(void) __asm__("_ZN22CompilerAuxiliaryScope6InsertEP14CompilerSymbol");
extern "C" void class_scope_slot_aux_3(void) __asm__("_ZN22CompilerAuxiliaryScope4FindEP17CompilerNameEntryj");
extern "C" void class_scope_slot_aux_4(void) __asm__("_ZN22CompilerAuxiliaryScope12FindOrdinaryEP17CompilerNameEntry");
extern "C" void class_scope_slot_aux_5(void) __asm__("_ZN22CompilerAuxiliaryScope6LookupEP17CompilerNameEntryjPj");
extern "C" void class_scope_slot_aux_6(void) __asm__("_ZN22CompilerAuxiliaryScope10FindNestedEP17CompilerNameEntryPP14CompilerSymbol");
extern "C" void class_scope_slot_aux_7(void) __asm__("_ZN22CompilerAuxiliaryScope8FindLinkEP18CompilerSymbolLink");
extern "C" void class_scope_slot_aux_8(void) __asm__("_ZN22CompilerAuxiliaryScope9SerializeEv");
extern "C" void class_scope_slot_aux_9(void) __asm__("_ZN22CompilerAuxiliaryScope5CheckEv");
extern "C" void class_scope_slot_aux_10(void) __asm__("_ZN22CompilerAuxiliaryScope4LinkEP14CompilerSymbol");
extern "C" int c1xx_test_class_scope_tables(void)
{
    union { DWORD align; BYTE bytes[32]; } storage;
    CompilerSymbol symbol;
    unsigned i;
    for (i = 0; i < 32; ++i) storage.bytes[i] = 0xa5;
    CompilerClassScope *scope = new(storage.bytes) CompilerClassScope(0xff, &symbol);
    CompilerAuxiliaryScope auxiliary(&symbol);
    union { DWORD align; BYTE bytes[32]; } parameter_storage;
    memset(parameter_storage.bytes, 0x91, sizeof(parameter_storage.bytes));
    CompilerAuxiliaryScope *parameter = new(parameter_storage.bytes) CompilerAuxiliaryScope(&symbol, 1);
    static void (*const class_expected[])(void) = {class_scope_slot_class_0,class_scope_slot_class_1,class_scope_slot_class_2,class_scope_slot_class_3,class_scope_slot_class_4,class_scope_slot_class_5,class_scope_slot_class_6,class_scope_slot_class_7,class_scope_slot_class_8,class_scope_slot_class_9,class_scope_slot_class_10};
    static void (*const aux_expected[])(void) = {class_scope_slot_aux_0,class_scope_slot_aux_1,class_scope_slot_aux_2,class_scope_slot_aux_3,class_scope_slot_aux_4,class_scope_slot_aux_5,class_scope_slot_aux_6,class_scope_slot_aux_7,class_scope_slot_aux_8,class_scope_slot_aux_9,class_scope_slot_aux_10};
    void **actual = *(void ***)scope;
    void **aux_actual = *(void ***)&auxiliary;
    for (i = 0; i < 11; ++i)
        if (actual[i] != (void *)class_expected[i] || aux_actual[i] != (void *)aux_expected[i]) return 0;
    for (i = 9; i < 12; ++i)
        if (storage.bytes[i] != 0xa5 || parameter_storage.bytes[i] != 0x91) return 0;
    for (i = 24; i < sizeof(parameter_storage.bytes); ++i)
        if (parameter_storage.bytes[i] != 0x91) return 0;
    if (*(void ***)parameter != aux_actual || parameter->level != 1 ||
        parameter->next || parameter->outer || parameter->context ||
        parameter->parameters != (CompilerParameterSet *)&symbol) return 0;
    return sizeof(CompilerScopeTable) == 20 && sizeof(CompilerScope) == 32 &&
        sizeof(*scope) == 32 && sizeof(auxiliary) == 24 &&
        !scope->next && scope->level == 0xff && !scope->outer && !scope->context &&
        scope->symbol == &symbol && !scope->auxiliary && !scope->reset_value &&
        !auxiliary.next && auxiliary.level == 0xfa && !auxiliary.outer &&
        !auxiliary.context && auxiliary.value == &symbol &&
        actual[0] == (void *)debug_table_target_2 && actual[1] == aux_actual[1];
}

extern "C" int c1xx_test_arena_storage_reset(void)
{
    BYTE bytes[33];
    CompilerArena arena;
    memset(bytes, 0xa5, sizeof(bytes));
    memset(&arena, 0, sizeof(arena));
    arena.base = bytes + 1;
    arena.cursor = bytes + 24;
    arena.committed_limit = bytes + 33;
    arena.committed_bytes = 32;
    frontend_data.recursion_depth = 1;
    arena.ResetStorage();
    if (arena.cursor != bytes + 24) return 0;
    for (unsigned i = 0; i < sizeof(bytes); ++i)
        if (bytes[i] != 0xa5) return 0;
    frontend_data.recursion_depth = 0;
    arena.ResetStorage();
    if (arena.cursor != arena.base || arena.committed_limit != bytes + 33 ||
        arena.committed_bytes != 32) return 0;
    for (unsigned i = 0; i < sizeof(bytes); ++i)
        if (bytes[i] != ((i >= 1 && i < 24) ? 0 : 0xa5)) return 0;
    return 1;
}

extern "C" int c1xx_test_qualifier_clear(void)
{
    struct GuardedQualifiers {
        DWORD before;
        CompilerDeclaratorQualifiers qualifiers;
        DWORD after;
    } storage;
    unsigned i;
    const BYTE *bytes = (const BYTE *)&storage.qualifiers;
    memset(&storage, 0xa7, sizeof(storage));
    storage.before = 0x1927ab59;
    storage.after = 0x3179c5a3;
    storage.qualifiers.Clear();
    for (i = 0; i < sizeof(storage.qualifiers); ++i)
        if (bytes[i]) return 0;
    if (storage.before != 0x1927ab59 || storage.after != 0x3179c5a3) return 0;
    memset(&storage.qualifiers, 0xa7, sizeof(storage.qualifiers));
    if (storage.qualifiers.Initialize(0x1739) != &storage.qualifiers ||
        storage.qualifiers.flags != 0x1739 || storage.qualifiers.value ||
        bytes[2] != 0xa7 || bytes[3] != 0xa7) return 0;
    {
        struct GuardedModifier { DWORD before; ParserTypeModifier value; DWORD after; } output;
        CompilerDeclaratorQualifiers right;
        BYTE *output_bytes = (BYTE *)&output.value;
        memset(&output, 0x91, sizeof(output));
        output.before = 0x3197ab59; output.after = 0x719ac3b5;
        storage.qualifiers.value = 0x1937ab59;
        right.flags = 0x3197;
        right.value = 0x5179ab31;
        output.value.Pointer(&storage.qualifiers, &right);
        if (output.value.kind != 1 || output.value.flags != 0x1739 ||
            output.value.qualifiers != 0x3197 || output.value.value != 0x1937ab59 ||
            output_bytes[6] != 0x91 || output_bytes[7] != 0x91 ||
            output.before != 0x3197ab59 || output.after != 0x719ac3b5) return 0;
        if (right.flags != 0x3197 || right.value != 0x5179ab31) return 0;
    }
    return storage.before == 0x1927ab59 && storage.after == 0x3179c5a3;
}

extern "C" int c1xx_test_parameter_description(void)
{
    struct GuardedDescription { DWORD before; CompilerParameterDescription value; DWORD after; } storage;
    CompilerParameterDescription expected;
    CompilerParameterSet parameters;
    CompilerDeclaratorQualifiers qualifiers;
    memset(&storage, 0xa7, sizeof(storage));
    memcpy(&expected, &storage.value, sizeof(expected));
    expected.parameters = &parameters;
    expected.kind = 3;
    expected.scope_open = 1;
    if (storage.value.Initialize(3, &parameters) != &storage.value ||
        memcmp(&storage.value, &expected, sizeof(expected))) return 0;
    memset(&qualifiers, 0x91, sizeof(qualifiers));
    qualifiers.flags = 0x1739;
    qualifiers.value = 0x5179ab31;
    memcpy(&expected.flags, &qualifiers, 8);
    storage.value.SetQualifiers(&qualifiers);
    if (memcmp(&storage.value, &expected, sizeof(expected))) return 0;
    expected.declaration_mode = 2;
    storage.value.SetDeclarationMode(2);
    if (memcmp(&storage.value, &expected, sizeof(expected))) return 0;
    expected.parameters = NULL; expected.kind = 5; expected.scope_open = 0;
    storage.value.Initialize(5, NULL);
    return !memcmp(&storage.value, &expected, sizeof(expected)) &&
        storage.before == 0xa7a7a7a7 && storage.after == 0xa7a7a7a7;
}
