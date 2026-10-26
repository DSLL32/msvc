#include <string.h>
#include "compiler-state.h"
#include "case.h"
#include <recon.h>

static int quoted_workspace_restore(void)
{
    unsigned i;
    CompilerSymbolBinding cached_tag, cached_enum;
    CompilerType enum_type;
    CompilerIrSymbol enum_symbol;
    if (c1_constant_diagnostic_mode != -1 || c1_data.enumerator_type ||
        c1_data.enumerator_value != -1 || c1_bss.current_tag_symbol || c1_data.parameter_sentinel || c1_data.pch_creation_name_seen || c1_data.data_linkage_option || c1_data.segment_defaults.type_context || c1_data.segment_defaults.fallback) return 0;
    if (c1_bss.browser_name_deferred || c1_data.unevaluated_type_depth || c1_data.void_type_list || c1_data.call_context) return 0;
    c1_data.void_type_list = (CompilerTypeList *)&enum_type;
    c1_data.call_context = (struct CompilerCallContext *)&enum_symbol;
    if (c1_bss.call_reference_depth) return 0;
    c1_bss.call_reference_depth = 13;
    if (c1_data.initializer_depth != -1 || c1_data.last_initializer_symbol ||
        c1_data.initializer_reset_first || c1_data.initializer_reset_second ||
        c1_data.initializer_reset_third) return 0;
    if (c1_data.record_sizes.known.static_symbol_bytes != 52 || c1_data.record_sizes.known.member_symbol_bytes != 52) return 0;
    c1_data.record_sizes.known.static_symbol_bytes = 71;
    c1_data.record_sizes.known.member_symbol_bytes = 73;
    if (c1_data.function_symbol_cache_index != 9 || c1_data.static_symbol_cache_index != 9) return 0;
    if (c1_bss.loop_labels || c1_bss.continue_labels || c1_bss.break_labels || c1_bss.switch_labels || c1_bss.default_labels ||
        c1_bss.label_stack_count) return 0;
    c1_bss.loop_labels = (struct ParserLabelStack *)&enum_symbol;
    c1_bss.continue_labels = (struct ParserLabelStack *)&enum_type;
    c1_bss.break_labels = (struct ParserLabelStack *)&cached_tag;
    c1_bss.switch_labels = (struct ParserLabelStack *)&cached_enum;
    c1_bss.default_labels = (struct ParserLabelStack *)&enum_symbol;
    c1_bss.label_stack_count = 13;
    c1_data.function_symbol_cache_index = 17;
    c1_data.static_symbol_cache_index = 19;
    c1_data.unevaluated_type_depth = 9;
    c1_data.initializer_depth = 7;
    c1_data.last_initializer_symbol = &enum_symbol;
    c1_data.initializer_reset_first = 11;
    c1_data.initializer_reset_second = 13;
    c1_data.initializer_reset_third = 17;
    c1_constant_diagnostic_mode = -7;
    c1_data.enumerator_type = &enum_type;
    c1_data.parameter_sentinel = &enum_symbol;
    c1_data.pch_creation_name_seen = 37;
    c1_data.data_linkage_option = 43;
    c1_data.segment_defaults.type_context = &enum_symbol;
    c1_data.segment_defaults.fallback = &enum_symbol;
    c1_data.enumerator_value = -113;
    if (c1_data.record_sizes.known.enumerator_symbol_bytes != 40 ||
        c1_data.record_sizes.known.tag_symbol_bytes != 52 ||
        c1_data.record_sizes.known.parameter_symbol_bytes != 52 || c1_data.parameter_symbol_cache_index != 9 ||
        c1_data.tag_symbol_cache_index != 10 || c1_data.symbol_cache.known.head10 || c1_data.enumerator_symbol_cache_index != 5 ||
        c1_data.symbol_cache.known.head5) return 0;
    c1_data.symbol_cache.known.head10 = &cached_tag;
    c1_data.symbol_cache.known.head5 = &cached_enum;
    if (c1_data.quoted_cursor != c1_bss.quoted_workspace) return 0;
    c1_data.quoted_cursor = c1_bss.quoted_workspace + 37;
    if (initialize_compiler_pass()) return 0;
    if (c1_bss.saved_data->quoted_cursor != c1_bss.quoted_workspace + 37) return 0;
    /* First entry saves data; only the reuse branch clears BSS. */
    if (c1_bss.loop_labels != (struct ParserLabelStack *)&enum_symbol ||
        c1_bss.continue_labels != (struct ParserLabelStack *)&enum_type ||
        c1_bss.break_labels != (struct ParserLabelStack *)&cached_tag ||
        c1_bss.switch_labels != (struct ParserLabelStack *)&cached_enum ||
        c1_bss.default_labels != (struct ParserLabelStack *)&enum_symbol ||
        c1_bss.label_stack_count != 13) return 0;
    memset(c1_bss.quoted_workspace, 0xa5, sizeof(c1_bss.quoted_workspace));
    c1_bss.conditional_states[0] = 0x12345678;
    c1_bss.macro_parameter_count = -17;
    c1_bss.initializer_symbol = &enum_symbol;
    c1_bss.initializer_bytes = 37;
    c1_bss.initializer_value_pending = 39;
    c1_bss.initializer_resume_label = (struct CompilerParserNode *)&enum_symbol;
    c1_bss.initializer_error_reported = 41;
    c1_bss.browser_name_deferred = 0xa5;
    c1_bss.initializer_saved_type = &enum_type;
    c1_data.record_sizes.known.static_symbol_bytes = 0;
    c1_data.record_sizes.known.member_symbol_bytes = 0;
    c1_data.function_symbol_cache_index = 0;
    c1_data.static_symbol_cache_index = 0;
    c1_data.unevaluated_type_depth = 0;
    c1_data.void_type_list = NULL;
    c1_data.call_context = NULL;
    c1_data.initializer_depth = 0;
    c1_data.last_initializer_symbol = NULL;
    c1_data.initializer_reset_first = 0;
    c1_data.initializer_reset_second = 0;
    c1_data.initializer_reset_third = 0;
    c1_bss.tag_declaration_seen = 37;
    c1_bss.current_tag_symbol = &enum_symbol;
    c1_bss.record_definitions[0].symbol = &enum_symbol;
    c1_bss.record_definitions[0].flags = 0xa5;
    c1_bss.record_definitions[0].alignment = 37;
    c1_constant_diagnostic_mode = 5;
    c1_data.enumerator_type = NULL;
    c1_data.parameter_sentinel = NULL;
    c1_data.pch_creation_name_seen = 0;
    c1_data.data_linkage_option = 0;
    c1_data.segment_defaults.type_context = NULL;
    c1_data.segment_defaults.fallback = NULL;
    c1_data.enumerator_value = 17;
    c1_data.quoted_cursor = NULL;
    c1_data.record_sizes.known.tag_symbol_bytes = 79;
    c1_data.record_sizes.known.parameter_symbol_bytes = 83;
    c1_data.parameter_symbol_cache_index = 17;
    c1_data.record_sizes.known.enumerator_symbol_bytes = 67;
    c1_data.tag_symbol_cache_index = 17;
    c1_data.symbol_cache.known.head10 = NULL;
    c1_data.symbol_cache.known.head5 = NULL;
    c1_data.enumerator_symbol_cache_index = 19;
    if (initialize_compiler_pass()) return 0;
    if (c1_bss.loop_labels || c1_bss.continue_labels || c1_bss.break_labels || c1_bss.switch_labels || c1_bss.default_labels ||
        c1_bss.label_stack_count) return 0;
    if (c1_data.record_sizes.known.enumerator_symbol_bytes != 40 ||
        c1_data.record_sizes.known.tag_symbol_bytes != 52 ||
        c1_data.record_sizes.known.parameter_symbol_bytes != 52 || c1_data.parameter_symbol_cache_index != 9 ||
        c1_data.tag_symbol_cache_index != 10 || c1_data.symbol_cache.known.head10 != &cached_tag ||
        c1_data.enumerator_symbol_cache_index != 5 || c1_data.symbol_cache.known.head5 != &cached_enum ||
        c1_constant_diagnostic_mode != -7 || c1_data.enumerator_type != &enum_type ||
        c1_data.parameter_sentinel != &enum_symbol || c1_data.pch_creation_name_seen != 37 || c1_data.data_linkage_option != 43 || c1_data.segment_defaults.type_context != &enum_symbol || c1_data.segment_defaults.fallback != &enum_symbol ||
        c1_data.enumerator_value != -113 || c1_bss.current_tag_symbol ||
        c1_bss.record_definitions[0].symbol || c1_bss.record_definitions[0].flags ||
        c1_bss.record_definitions[0].alignment) return 0;
    if (c1_data.quoted_cursor != c1_bss.quoted_workspace + 37 ||
        c1_bss.conditional_states[0] || c1_bss.macro_parameter_count || c1_bss.tag_declaration_seen) return 0;
    if (c1_data.record_sizes.known.static_symbol_bytes != 71 || c1_data.record_sizes.known.member_symbol_bytes != 73) return 0;
    if (c1_data.void_type_list != (CompilerTypeList *)&enum_type) return 0;
    if (c1_data.call_context != (struct CompilerCallContext *)&enum_symbol) return 0;
    if (c1_bss.call_reference_depth) return 0;
    if (c1_data.unevaluated_type_depth != 9 || c1_data.function_symbol_cache_index != 17 ||
        c1_data.static_symbol_cache_index != 19) return 0;
    if (c1_data.initializer_depth != 7 || c1_data.last_initializer_symbol != &enum_symbol ||
        c1_data.initializer_reset_first != 11 || c1_data.initializer_reset_second != 13 ||
        c1_data.initializer_reset_third != 17 || c1_bss.initializer_symbol ||
        c1_bss.initializer_bytes || c1_bss.initializer_value_pending || c1_bss.initializer_resume_label ||
        c1_bss.initializer_error_reported || c1_bss.initializer_saved_type || c1_bss.browser_name_deferred) return 0;
    for (i = 0; i < sizeof(c1_bss.quoted_workspace); ++i)
        if (c1_bss.quoted_workspace[i]) return 0;
    return 1;
}
extern void __cdecl copy_numeric_spelling(CompilerValue *, const char *, unsigned, unsigned);
static int literal_spelling_write_extent(void)
{
    BYTE storage[16];
    CompilerValue literal;
    unsigned i;
    memset(storage, 0xa5, sizeof(storage));
    memset(&literal, 0x6b, sizeof(literal));
    c1_bss.arenas[0].cursor = storage;
    c1_bss.arenas[0].committed_limit = storage + sizeof(storage);
    copy_numeric_spelling(&literal, "ab", 0x10003, 0);
    if (literal.string_attributes.text != (const char *)storage ||
        literal.string_attributes.length != 3 || c1_bss.arenas[0].cursor != storage + 4)
        return 0;
    if (memcmp(storage, "ab", 3)) return 0;
    for (i = 3; i < sizeof(storage); ++i) if (storage[i] != 0xa5) return 0;
    for (i = 6; i < sizeof(literal); ++i)
        if (((BYTE *)&literal)[i] != 0x6b) return 0;
    return 1;
}
extern int c1_test_include_alias_lookup(void);
extern int c1_test_recorded_lexer_initialize(void);
extern void __cdecl parse_macro_parameters(void);
extern unsigned __cdecl read_macro_replacement(void);
extern int __cdecl find_macro_parameter(const char *);
static int macro_parameter_encoding(void)
{
    BYTE storage[64];
    BYTE input[] = "alpha,beta)";
    BYTE replacement[] = " alpha+beta\n";
    static const BYTE records[] = {7,'a','l','p','h','a',0,6,'b','e','t','a',0};
    static const BYTE encoded[] = {1,1,'+',1,2,0,0x44};
    unsigned i;
    memset(storage, 0xa5, sizeof(storage));
    c1_data.options.language_extensions = 1;
    c1_bss.source_cursor = input;
    c1_bss.work_buffers[1].data = storage;
    c1_bss.work_buffers[1].limit = storage + sizeof(storage) - 5;
    c1_bss.current_macro_text = (const char *)storage;
    c1_bss.macro_parameter_count = 0;
    parse_macro_parameters();
    if (c1_bss.macro_parameter_count != 2 ||
        c1_bss.current_macro_text != (const char *)(storage + sizeof(records)) ||
        memcmp(storage, records, sizeof(records)) ||
        find_macro_parameter("alpha") != 1 || find_macro_parameter("beta") != 2 ||
        find_macro_parameter("ALPHA") != -1 || find_macro_parameter("gamma") != -1) return 0;
    c1_bss.source_cursor = replacement;
    if (read_macro_replacement() != sizeof(encoded) ||
        memcmp(storage + sizeof(records), encoded, sizeof(encoded)) ||
        memcmp(storage, records, sizeof(records)) || *c1_bss.source_cursor != '\n') return 0;
    /* Parameter recognition temporarily writes beta's NUL after its full
     * spelling, then shortens that spelling to a two-byte reference. That
     * defined scratch byte is beyond the final returned output length. */
    if (storage[sizeof(records) + sizeof(encoded)] != 0) return 0;
    for (i = sizeof(records) + 2 + 1 + sizeof("beta"); i < sizeof(storage); ++i)
        if (storage[i] != 0xa5) return 0;
    c1_bss.macro_parameter_count = 0;
    c1_bss.work_buffers[1].data = NULL;
    return find_macro_parameter("alpha") == -1;
}
extern void __cdecl set_declaration_storage(CompilerDeclarationAttributes *, unsigned, unsigned, ...);
static int declaration_attribute_writes(void)
{
    struct { unsigned before; CompilerDeclarationAttributes attributes; unsigned after; } record;
    CompilerIrSymbol segment;
    record.before = 0x93b27d41;
    record.after = 0x827a4e19;
    record.attributes.flags = 0xa5f8;
    record.attributes.extended_flags = 0x624b;
    record.attributes.extra = 0x12345678;
    /* The genuine typedef spelling has only three arguments. */
    set_declaration_storage(&record.attributes, 7, 1);
    if (record.attributes.flags != 0xa5ff || record.attributes.extended_flags != 0x624b ||
        record.attributes.extra != 0x12345678) return 0;
    record.attributes.flags = 0x8000;
    record.attributes.extended_flags = 0x8200;
    set_declaration_storage(&record.attributes, 0, 8, &segment);
    if (record.attributes.flags != 0x8000 || record.attributes.extended_flags != 0x8300 ||
        record.attributes.allocated_segment != &segment) return 0;
    record.attributes.extended_flags = 0x8200;
    set_declaration_storage(&record.attributes, 0, 8, (CompilerIrSymbol *)0);
    if (record.attributes.extended_flags != 0x8200 || record.attributes.allocated_segment) return 0;
    set_declaration_storage(&record.attributes, 0, 3);
    return record.attributes.flags == 0x8100 && record.before == 0x93b27d41 && record.after == 0x827a4e19;
}
extern void *__cdecl create_enum_context(unsigned, unsigned, unsigned, unsigned, int, int);
static int enum_context_write_extent(void)
{
    struct { unsigned before; BYTE storage[64]; unsigned after; } owner;
    CompilerEnumContext *context;
    unsigned i;
    memset(&owner, 0xa6, sizeof(owner));
    c1_bss.arenas[0].cursor = owner.storage;
    c1_bss.arenas[0].committed_limit = owner.storage + sizeof(owner.storage);
    context = (CompilerEnumContext *)create_enum_context(4, 0, 53, 91, 1, 0);
    if ((BYTE *)context != owner.storage || c1_bss.arenas[0].cursor != owner.storage + 40 ||
        context->arena_kind || context->hash_kind != 53 || context->match_kind != 91 ||
        context->capacity != 4 || context->flags != 0xa5 || context->padding_0f != 0xa6 ||
        context->symbols != (CompilerEnumEntry **)(owner.storage + 24) || context->count ||
        owner.before != 0xa6a6a6a6 || owner.after != 0xa6a6a6a6) return 0;
    for (i = 24; i < sizeof(owner.storage); ++i) if (owner.storage[i] != 0xa6) return 0;
    return 1;
}
extern CompilerIrSymbol *__cdecl lookup_cached_symbol_record(unsigned, struct CompilerNameEntry *, unsigned);
static int enum_cache_write_extent(void)
{
    struct { unsigned before; BYTE record[40]; unsigned after; } owner;
    CompilerSymbolBinding *binding = (CompilerSymbolBinding *)(owner.record + 8), next;
    BYTE name_storage[16];
    struct CompilerNameEntry *name = (struct CompilerNameEntry *)name_storage;
    CompilerIrSymbol *result;
    unsigned i;
    memset(&owner, 0xa7, sizeof(owner));
    binding->next = &next;
    c1_data.symbol_cache.known.head5 = binding;
    if (lookup_cached_symbol_record(5, name, 2) || c1_data.symbol_cache.known.head5 != binding) return 0;
    result = lookup_cached_symbol_record(5, name, 0);
    if ((BYTE *)result != owner.record || result->kind != 5 || result->binding.name != name ||
        c1_data.symbol_cache.known.head5 != &next || owner.before != 0xa7a7a7a7 || owner.after != 0xa7a7a7a7) return 0;
    for (i = 1; i < 40; ++i) if ((i < 8 || i >= 12) && owner.record[i]) return 0;
    c1_data.symbol_cache.known.head5 = NULL;
    if (lookup_cached_symbol_record(5, name, 0) != NULL) return 0;
    {
        struct { unsigned before; BYTE record[52]; unsigned after; } formal;
        CompilerSymbolBinding *head = (CompilerSymbolBinding *)(formal.record + 8);
        memset(&formal, 0xa8, sizeof(formal));
        head->next = NULL;
        c1_data.symbol_cache.known.head9 = head;
        if (lookup_cached_symbol_record(12, name, 2) || c1_data.symbol_cache.known.head9 != head) return 0;
        result = lookup_cached_symbol_record(12, name, 0);
        if ((BYTE *)result != formal.record || result->kind != 12 || result->binding.name != name ||
            c1_data.symbol_cache.known.head9 || formal.before != 0xa8a8a8a8 || formal.after != 0xa8a8a8a8) return 0;
        for (i = 1; i < 52; ++i) if ((i < 8 || i >= 12) && formal.record[i]) return 0;
    }
    return 1;
}
extern int c1_test_record_definition_write_extent(void);
extern int __cdecl integer_sum_out_of_range(__int64, unsigned __int64, WORD, WORD);
static int integer_sum_boundaries(void)
{
    static const struct SumCase { __int64 first; unsigned __int64 second; WORD unsign, width; int overflow; } cases[] = {
        { 0xfffffffeui64, 1, 1, 4, 0 }, { 0xffffffffui64, 1, 1, 4, 1 },
        { 0x100000000ui64, 0, 1, 4, 0 }, { 255, 1, 1, 1, 1 },
        { 127, 1, 0, 1, 1 }, { -128, (unsigned __int64)-1, 0, 1, 1 },
        { 2147483647, 1, 0, 4, 1 }, { -2147483647i64-1, (unsigned __int64)-1, 0, 4, 1 },
        { -7, 2, 0, 4, 0 }, { 7, (unsigned __int64)-2, 0, 4, 0 },
        { 0x7fffffffffffffffi64, 1, 0, 8, 1 },
        { (-0x7fffffffffffffffi64-1), (unsigned __int64)-1, 0, 8, 1 }
    };
    unsigned i;
    for (i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i)
        if (integer_sum_out_of_range(cases[i].first, cases[i].second,
            cases[i].unsign, cases[i].width) != cases[i].overflow) return 0;
    return 1;
}
extern int c1_test_parameter_link_write_extent(void);
extern void __cdecl push_macro_expansion(CompilerMacro *);
extern int __cdecl macro_is_active(CompilerMacro *);
extern void __cdecl begin_macro_replacement(void);
extern int __cdecl finish_macro_replacement(void);
extern unsigned __cdecl process_special_character(void);
static int macro_frame_write_extent(void)
{
    BYTE source[8], arguments[32], replacement[32];
    CompilerMacro macro, other;
    CompilerMacroBody body;
    CompilerMacroExpansionFrame *frame = &c1_bss.macro_expansions[1];
    memset(frame, 0xa5, sizeof(*frame));
    c1_bss.macro_expansions[0].macro = NULL;
    c1_bss.macro_expansions[0].active = 0;
    c1_bss.lexical_skip_mode = 0;
    c1_bss.macro_active_count = 3;
    c1_bss.saved_macro_token_cursor = arguments;
    c1_bss.saved_macro_text_cursor = replacement;
    c1_bss.source_cursor = source;
    c1_bss.macro_argument_count = 0x123401fe;
    push_macro_expansion(&macro);
    if (c1_bss.lexical_skip_mode != 1 || c1_bss.macro_active_count != 4 ||
        frame->macro != &macro || frame->source != source || frame->arguments != arguments ||
        frame->replacement != replacement || frame->argument_count != -2 || frame->argument_index ||
        c1_bss.macro_text_cursor != replacement || frame->active != 0xa5a5a5a5 ||
        frame->unmodeled_12[0] != 0xa5 || frame->unmodeled_12[1] != 0xa5) return 0;
    frame->active = 0;
    if (macro_is_active(&macro)) return 0;
    frame->active = 1;
    if (!macro_is_active(&macro) || macro_is_active(&other)) return 0;
    body.kind = 0; body.payload[0] = '4'; macro.body = &body;
    c1_data.options.language_extensions = 0;
    begin_macro_replacement();
    if (c1_bss.source_cursor != body.payload || frame->active != 1) return 0;
    memset(replacement, 0xa6, sizeof(replacement));
    c1_bss.macro_active_count = 1;
    c1_bss.work_buffers[0].data = arguments;
    c1_bss.work_buffers[1].data = replacement;
    c1_bss.work_buffers[1].limit = replacement + 6;
    if (!finish_macro_replacement() || c1_bss.source_cursor != replacement ||
        c1_bss.macro_text_cursor != replacement + 2 || replacement[0] || replacement[1] != 'R' ||
        replacement[2] != 0xa6 || !c1_bss.macro_rescan_output_pending) return 0;
    c1_bss.source_cursor = replacement + 1;
    if (process_special_character() != 0x20 || c1_bss.lexical_skip_mode ||
        c1_bss.source_cursor != source || frame->active || c1_bss.macro_rescan_output_pending ||
        c1_bss.macro_text_cursor != replacement || c1_bss.macro_token_cursor != arguments) return 0;
    /* Argument completion saves a length-prefixed stream and restores the
     * output cursor, independently of the next source argument cursor. */
    memset(arguments, 0xa5, sizeof(arguments));
    memcpy(replacement, "abc", 3);
    source[0] = 0; source[1] = 'A';
    c1_bss.lexical_skip_mode = 1;
    frame->arguments = arguments;
    frame->argument_count = 1;
    frame->argument_index = 0;
    c1_bss.macro_token_cursor = arguments;
    c1_bss.work_buffers[0].limit = arguments + sizeof(arguments);
    c1_bss.macro_text_cursor = replacement + 3;
    c1_bss.source_cursor = source + 1;
    if (process_special_character() != 2 ||
        c1_bss.macro_text_cursor != replacement ||
        c1_bss.macro_token_cursor != arguments + 7 ||
        c1_bss.source_cursor != arguments + 2 || frame->argument_index != 1 ||
        *(WORD *)arguments != 7 || memcmp(arguments + 2, "abc", 3) ||
        arguments[5] || arguments[6] != 'A' || arguments[7] != 0xa5) return 0;
    return frame->unmodeled_12[0] == 0xa5 && frame->unmodeled_12[1] == 0xa5;
}
extern void __cdecl write_initializer_integer(const unsigned __int64 *, unsigned, CompilerOutputBuffer *);
static int initializer_integer_boundaries(void)
{
    static const __int64 numbers[] = { -128, -127, -1, 0, 127, 128, 65536, 0x123456789LL };
    static const unsigned widths[] = { 2, 4, 8 };
    unsigned i, j, k, width, count;
    unsigned __int64 raw;
    int compact;
    BYTE storage[32], expected[9];
    CompilerOutputBuffer output;
    for (i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i) {
        raw = (unsigned __int64)numbers[i];
        for (j = 0; j < sizeof(widths) / sizeof(widths[0]); ++j) {
            width = widths[j];
            compact = width == 8 ? numbers[i] >= -127 && numbers[i] <= 127 :
                (width == 2 ? (unsigned short)raw : (unsigned)raw) <= 127;
            count = compact ? 1 : width + 1;
            expected[0] = compact ? (BYTE)raw : 0x80;
            if (!compact) memcpy(expected + 1, &raw, width);
            memset(storage, 0xa5, sizeof(storage));
            memset(&output, 0xa5, sizeof(output));
            output.data = output.cursor = storage + 4;
            output.capacity = 24;
            output.good = 1;
            write_initializer_integer(&raw, width, &output);
            if (output.cursor != storage + 4 + count || output.good != 1 ||
                memcmp(storage + 4, expected, count)) return 0;
            for (k = 0; k < 4; ++k) if (storage[k] != 0xa5) return 0;
            for (k = 4 + count; k < sizeof(storage); ++k) if (storage[k] != 0xa5) return 0;
        }
    }
    return 1;
}
extern void __cdecl finish_initializer_record(void);
static int initializer_record_termination(void)
{
    BYTE storage[17];
    CompilerOutputBuffer *output = &c1_bss.output_buffers[4];
    int good, bypass;
    unsigned i;
    for (bypass = 0; bypass < 2; ++bypass) for (good = 0; good < 4; ++good) {
        memset(storage, 0xa5, sizeof(storage));
        output->data = storage;
        output->cursor = storage + 4;
        output->capacity = sizeof(storage);
        output->good = good;
        c1_data.function_record_flags = bypass ? 0x800 : 0;
        finish_initializer_record();
        if (output->good != (bypass ? good : (good & 1)) ||
            output->cursor != storage + 4 + (!bypass && (good & 1))) return 0;
        for (i = 0; i < sizeof(storage); ++i)
            if (storage[i] != ((!bypass && (good & 1) && i == 4) ? 7 : 0xa5)) return 0;
        if (c1_data.function_record_flags != (bypass ? 0x800U : 0)) return 0;
    }
    return 1;
}
extern void __cdecl discard_declaration_symbol(CompilerIrSymbol *);
extern void __cdecl copy_declaration_properties(CompilerIrSymbol *, CompilerIrSymbol *);
static int declaration_cache_extent(void)
{
    CompilerIrSymbol symbol, expected;
    CompilerSymbolBinding old_head;
    unsigned kind;
    for (kind = 7; kind <= 10; kind += 3) {
        memset(&symbol, 0xa5, sizeof(symbol));
        symbol.kind = (BYTE)kind;
        expected = symbol;
        c1_bss.parameter_nesting = 0;
        c1_data.symbol_cache.known.head9 = &old_head;
        expected.binding.next = &old_head;
        discard_declaration_symbol(&symbol);
        if (memcmp(&symbol, &expected, sizeof(symbol)) ||
            c1_data.symbol_cache.known.head9 != &symbol.binding) return 0;
        c1_bss.parameter_nesting = 1;
        c1_data.symbol_cache.known.head9 = &old_head;
        discard_declaration_symbol(&symbol);
        if (memcmp(&symbol, &expected, sizeof(symbol)) ||
            c1_data.symbol_cache.known.head9 != &old_head) return 0;
    }
    return 1;
}
static int declaration_merge_extent(void)
{
    CompilerIrSymbol previous, added, expected, saved_added;
    struct CompilerTypeShape old_shape, new_shape;
    CompilerType old_type, new_type;
    memset(&previous, 0xa5, sizeof(previous));
    memset(&added, 0x5a, sizeof(added));
    memset(&old_type, 0, sizeof(old_type));
    memset(&new_type, 0, sizeof(new_type));
    memset(&old_shape, 0, sizeof(old_shape));
    memset(&new_shape, 0, sizeof(new_shape));
    old_shape.flags = new_shape.flags = 2;
    new_shape.value = 3;
    old_type.shape = &old_shape;
    new_type.shape = &new_shape;
    previous.kind = added.kind = 10;
    previous.flags = 2;
    previous.declaration_flags = 0;
    added.flags = 0;
    added.declaration_flags = 4;
    previous.scope_bits = added.scope_bits = 0;
    previous.value_type = &old_type;
    added.value_type = &new_type;
    previous.intrinsic_flags = 3;
    added.intrinsic_flags = 0x00408043;
    expected = previous;
    saved_added = added;
    expected.value_type = &new_type;
    expected.qualifier_flags &= ~1;
    expected.declaration_flags |= 4;
    expected.intrinsic_flags = 0x00408043;
    expected.flags &= ~2;
    copy_declaration_properties(&previous, &added);
    return !memcmp(&previous, &expected, sizeof(previous)) &&
        !memcmp(&added, &saved_added, sizeof(added));
}
extern int c1_test_initializer_copy_extent(void);
extern int c1_test_unary_node_extent(void);
extern int c1_test_binary_comparison_signedness(void);
extern int c1_test_integer_conversion_ranges(void);
extern int __cdecl integer_product_out_of_range(__int64, unsigned __int64, WORD, WORD);
static int product_range_boundaries(void)
{
    unsigned __int64 high_bit = (unsigned __int64)1 << 63;
    if (integer_product_out_of_range(2147483647, 1, 0, 4) ||
        !integer_product_out_of_range(2147483647, 2, 0, 4) ||
        integer_product_out_of_range(-2147483647 - 1, 1, 0, 4) ||
        !integer_product_out_of_range(-2147483647 - 1, (unsigned __int64)-1, 0, 4)) return 0;
    if (integer_product_out_of_range(0, high_bit, 0, 8) ||
        integer_product_out_of_range(1, high_bit, 0, 8) ||
        !integer_product_out_of_range(-1, high_bit, 0, 8)) return 0;
    if (integer_product_out_of_range((__int64)0xffffffffU, 1, 1, 4) ||
        !integer_product_out_of_range((__int64)0xffffffffU, 2, 1, 4) ||
        integer_product_out_of_range(-1, 1, 1, 8) ||
        !integer_product_out_of_range(-1, 2, 1, 8)) return 0;
    /* The unsigned branch truncates operands to the selected width first. */
    if (integer_product_out_of_range(257, 255, 1, 1) ||
        !integer_product_out_of_range(255, 2, 1, 1) ||
        integer_product_out_of_range(-128, 1, 0, 1) ||
        !integer_product_out_of_range(-128, (unsigned __int64)-1, 0, 1)) return 0;
    return 1;
}
extern int c1_test_parser_label_link_extent(void);
extern unsigned __cdecl process_trigraph(void);
static int trigraph_cursor_boundaries(void)
{
    BYTE ordinary[] = { '?', 'x', 0xa5 };
    BYTE converted[] = { '?', '?', '/', 0xa5 };
    BYTE overlap[] = { '?', '?', 'x', 0xa5 };
    BYTE *saved = c1_bss.source_cursor;
    int skipped = c1_bss.lexical_skip_mode;
    c1_bss.lexical_skip_mode = 0;
    c1_bss.source_cursor = ordinary + 1;
    if (process_trigraph() != 0x40 || c1_bss.source_cursor != ordinary + 1 ||
        ordinary[0] != '?' || ordinary[1] != 'x' || ordinary[2] != 0xa5) return 0;
    c1_bss.source_cursor = converted + 1;
    if (process_trigraph() != 1 || c1_bss.source_cursor != converted + 2 ||
        converted[0] != '?' || converted[1] != '?' || converted[2] != '\\' ||
        converted[3] != 0xa5) return 0;
    c1_bss.source_cursor = overlap + 1;
    if (process_trigraph() != 0x40 || c1_bss.source_cursor != overlap + 1 ||
        overlap[0] != '?' || overlap[1] != '?' || overlap[2] != 'x' || overlap[3] != 0xa5) return 0;
    c1_bss.source_cursor = saved;
    c1_bss.lexical_skip_mode = skipped;
    return 1;
}
const ReconTest language_tests[23] = {
    { "quoted cursor snapshot preserves an owned offset and clears the whole spelling workspace", 1, quoted_workspace_restore },
    { "literal spelling copies WORD length, aligns allocation and preserves flags and tail", 1, literal_spelling_write_extent },
    { "include aliases select case-insensitive name and kind without changing misses", 1, c1_test_include_alias_lookup },
    { "recorded lexer initialization preserves unknown bits and owned boundaries", 1, c1_test_recorded_lexer_initialize },
    { "macro parameters encode ordered references and preserve record and output tails", 1, macro_parameter_encoding },
    { "declaration attributes respect three-argument storage and attribute-specific fourth argument", 1, declaration_attribute_writes },
    { "enum context writes known fields while preserving flag bits padding and vector contents", 1, enum_context_write_extent },
    { "symbol caches pop and clear only their authentic forty- or fifty-two-byte owners", 1, enum_cache_write_extent },
    { "record definition initializes only its actual frame and twelve-byte member owner", 1, c1_test_record_definition_write_extent },
    { "integer sum range preserves signed and unsigned boundary behavior", 1, integer_sum_boundaries },
    { "parameter binding construction preserves symbol fields and twelve-byte allocation boundaries", 1, c1_test_parameter_link_write_extent },
    { "macro rescan frame preserves padding and returns through its genuine stream markers", 1, macro_frame_write_extent },
    { "initializer symbol copy retains opaque payload and exact fifty-two-byte boundaries", 1, c1_test_initializer_copy_extent },
    { "initializer integers preserve width-specific compact boundaries and output tails", 1, initializer_integer_boundaries },
    { "unary and enum node constructors preserve unknown fields and allocation boundaries", 1, c1_test_unary_node_extent },
    { "initializer terminator preserves output tails and flag-dependent cursor effects", 1, initializer_record_termination },
    { "declaration recycling changes only the embedded link and respects parameter scope", 1, declaration_cache_extent },
    { "declaration merging completes an array bound and preserves unrelated symbol bytes", 1, declaration_merge_extent },
    { "binary constant comparisons distinguish signed and unsigned boundaries and preserve node tails", 1, c1_test_binary_comparison_signedness },
    { "integer conversion ranges retain signed limits and unsigned magnitude rules", 1, c1_test_integer_conversion_ranges },
    { "parser label link preserves the unwritten node and exact eight-byte arena extent", 1, c1_test_parser_label_link_extent },
    { "integer product range handles signed minima unsigned limits zero and narrowed operands", 1, product_range_boundaries },
    { "trigraph cursor conversion and overlapping rollback preserve neighboring bytes", 1, trigraph_cursor_boundaries }
};
