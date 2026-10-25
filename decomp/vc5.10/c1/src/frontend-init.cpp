#include <io.h>
#include <new.h>
#include <string.h>
#include "frontend.h"
#include "packing.h"
#pragma intrinsic(memset, memcpy, memcmp)
#include <recon.h>

extern "C" {
int __cdecl create_input(int, CompilerInput **, CompilerTextInput **);
void __cdecl normalize_frontend_options(void);
void __cdecl format_processor_switch(char *, const char *, unsigned, int);
void __cdecl begin_browser_output(const char *);
CompilerBackend *__cdecl acquire_initial_backend(void);
int __cdecl prepare_pch_input(const char *);
void __cdecl read_initial_input(void);
void __cdecl initialize_predefined_tokens(void);
void __cdecl initialize_keyword_table(void);
void __cdecl initialize_language_keywords(void);
void __cdecl initialize_line_tables(void);
void __cdecl open_expression_output(void);
void __cdecl initialize_type_table(void);
CompilerType *__cdecl intern_type(CompilerType *);
void __cdecl initialize_global_scope(void);
void __cdecl initialize_builtin_symbols(void);
CompilerType *__cdecl make_function_type(unsigned, CompilerType *);
CompilerType *__cdecl make_tagged_type(unsigned);
void __cdecl initialize_intermediate_output(void);
void __cdecl initialize_translation_state(void);
void __cdecl define_command_line_macros(CompilerVector *);
void __cdecl begin_line_output(void);
void __cdecl begin_source_input(void);
void __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl initialize_frontend(void)
{
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerType *description;
    CompilerBuiltinTypes *types = &c1_data.builtin_types;
    CompilerLexer *lexer;
    CompilerLexerBackend *backend;
    void *memory;
    unsigned alignment;
    unsigned short integer_flags, unsigned_integer_flags;
    int record;
    char processor_switch[20];

    record = c1_data.options.incremental_compile &&
        !c1_bss.options.preprocess_stdout && !c1_bss.options.prototypes_only;
    if (!create_input(record, &c1_bss.input, &c1_bss.text_input))
        compiler_error(73, ".\\main.c", 178);
    SetHandleCount(30);
    c1_data.options.preprocess_no_lines = c1_data.options.preprocess_no_lines ||
        !c1_bss.options.preprocess_stdout;
    alignment = c1_data.options.packing_alignment;
    if (alignment == (unsigned)-1)
        alignment = 8;
    c1_data.options.packing_stack->active->alignment = alignment;
    c1_data.normalized_stack_checks = c1_data.options.stack_checks;
    c1_data.normalized_oq = c1_data.options.option_oq;
    c1_data.normalized_pool_strings = c1_data.options.pool_strings;
    c1_data.normalized_readonly_strings = c1_data.options.readonly_strings;
    if ((unsigned)c1_data.options.processor > 6) {
        format_processor_switch(processor_switch, "-G%d", 20, c1_data.options.processor);
        compiler_error(7, processor_switch, "c1");
    } else if ((unsigned)c1_data.options.processor > 4) {
        c1_data.options.processor = 4;
    }
    normalize_frontend_options();
    if (c1_bss.options.preprocess_stdout || c1_bss.options.prototypes_only) {
        c1_bss.options.browser_file = NULL;
        c1_bss.options.browser_local_file = NULL;
        c1_data.browser_full = 0;
        c1_data.browser_enabled = 0;
    } else {
        if (c1_bss.options.browser_file) {
            c1_bss.options.browser_local_file = c1_bss.options.browser_file;
            c1_data.browser_full = 1;
        }
        c1_data.browser_enabled = c1_bss.options.browser_local_file != NULL;
    }
    c1_bss.browser_filename = c1_bss.options.browser_local_file;
    begin_browser_output(c1_bss.options.browser_local_file);
    c1_data.backend = acquire_initial_backend();
    if (c1_data.options.pch_no_preprocess) {
        if (c1_data.options.pch_create_file ||
            (c1_data.options.pch_file && !c1_data.options.pch_use)) {
            if (prepare_pch_input(c1_data.options.pch_create_file)) {
                c1_bss.pch_replayed = 1;
                c1_bss.pch_input_ready = c1_data.options.pch_use;
                c1_bss.pch_input_file = c1_data.options.pch_use_file;
                if (c1_data.options.pch_use)
                    c1_data.options.pch_file = NULL;
                c1_data.options.pch_use = 1;
                c1_data.options.pch_use_file = c1_data.options.pch_create_file;
                c1_data.options.pch_create = 0;
                c1_data.options.pch_create_file = NULL;
            } else {
                c1_bss.pch_file_failed = 1;
                read_initial_input();
            }
        } else {
            c1_data.options.pch_no_preprocess = 0;
        }
    }
    c1_data.saved_pch_mode = c1_data.options.pch_create;
    c1_data.pch_separate_debug = c1_data.options.debug_info &&
        !c1_data.options.pch_debug && c1_data.options.pch_create;
    initialize_predefined_tokens();
    initialize_keyword_table();
    initialize_language_keywords();
    initialize_line_tables();
    if (!c1_bss.options.preprocess_stdout && !c1_bss.options.prototypes_only) {
        if (c1_data.c9_il) {
            open_expression_output();
            c1_bss.output_buffers[1].Open(c1_data.options.il_prefix, "sy", "w+b");
            c1_bss.output_buffers[2].Open(c1_data.options.il_prefix, "gl", "w+b");
            c1_bss.output_buffers[4].Open(c1_data.options.il_prefix, "in", "w+b");
            if (c1_data.options.debug_info)
                c1_bss.output_buffers[0].Open(c1_data.options.il_prefix, "db", "w+b");
        } else {
            c1_bss.output_kinds[0] = 3; /* RVA54c68 */
            c1_bss.output_kinds[1] = 1; /* RVA53fe8 */
            c1_bss.output_kinds[2] = 4; /* RVA54f18 */
            c1_bss.output_kinds[3] = 2; /* RVA54a50 */
            c1_bss.output_kinds[4] = 1; /* RVA547e8 */
            c1_bss.output_kinds[5] = 5; /* RVA53eb8 */
        }
    }
    initialize_type_table();
    arena = &c1_bss.arenas[3];
    description = (CompilerType *)arena->cursor;
    arena->cursor += sizeof(CompilerType);
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    description->attributes = 0;
    description->shape = NULL;
    description->auxiliary = 0;
    /* The original does not initialize the debug-index cache here. */
    integer_flags = ((c1_bss.options.prototypes_only != 0) ? 0x20 : 0) | 0x63;
    description->flags = 0;
    types->type_0000 = intern_type(description);
    description->flags = 0x4041;
    types->type_4041_first = intern_type(description);
    description->flags = integer_flags;
    types->type_0063 = intern_type(description);
    unsigned_integer_flags = integer_flags | 0x4000;
    description->flags = unsigned_integer_flags;
    types->type_4063_first = intern_type(description);
    description->flags = 0x41;
    types->type_0041 = intern_type(description);
    description->flags = 0x4041;
    types->type_4041_second = intern_type(description);
    description->flags = 0x42;
    types->type_0042 = intern_type(description);
    description->flags = 0x4042;
    types->type_4042_first = intern_type(description);
    description->flags = 0x43;
    types->type_0043 = intern_type(description);
    description->flags = 0x4043;
    types->type_4043 = intern_type(description);
    description->flags = 0x4c;
    types->type_004c = intern_type(description);
    description->flags = 0x404c;
    types->type_404c = intern_type(description);
    description->flags = 0x40;
    types->type_0040 = intern_type(description);
    description->flags = unsigned_integer_flags;
    types->type_4063_second = intern_type(description);
    description->flags = 0x42;
    types->type_0042_second = intern_type(description);
    description->flags = 0x4042;
    types->type_4042_second = intern_type(description);
    description->flags = 0x44;
    types->type_0044 = intern_type(description);
    description->flags = 0x4044;
    types->type_4044 = intern_type(description);
    description->flags = 0x106;
    types->type_0106 = intern_type(description);
    description->flags = 0x107;
    types->type_0107 = intern_type(description);
    description->flags = 0x108;
    types->type_0108 = intern_type(description);
    description->flags = 0x802;
    types->type_0802 = intern_type(description);
    description->flags = 0x80;
    types->type_0080 = intern_type(description);
    description->attributes = 0x100;
    description->flags = 0x41;
    types->type_0041_q0100 = intern_type(description);
    description->flags = 0x4041;
    types->type_4041_q0100 = intern_type(description);
    description->flags = integer_flags;
    types->type_0063_q0100 = intern_type(description);
    description->attributes = 0;
    initialize_global_scope();
    initialize_builtin_symbols();
    pool = &c1_bss.pools[7];
    if (pool->remaining < 540)
        pool->Refill(540);
    memory = pool->cursor;
    pool->cursor += 540;
    pool->remaining -= 540;
    /* The original allocation is540 bytes; unwritten scope entries remain unknown. */
    c1_data.translation_unit = memory ?
        ((CompilerTranslationUnit *)memory)->Initialize() : NULL;
    if (c1_data.dependency_local || c1_data.options.dependency_output) {
        arena = &c1_bss.arenas[1];
        memory = arena->cursor;
        arena->cursor += sizeof(CompilerDependency);
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        if (memory) {
            c1_data.dependency = (CompilerDependency *)memory;
            c1_data.dependency->records = NULL;
        } else {
            c1_data.dependency = NULL;
        }
    }
    if (!c1_bss.options.preprocess_stdout) {
        types->implicit_int_function = make_function_type(0, types->type_0063);
        types->tagged_0080 = make_tagged_type(0x80);
        if (!c1_bss.options.prototypes_only)
            initialize_intermediate_output();
        initialize_translation_state();
    }
    define_command_line_macros(c1_bss.options.defines);
    if (c1_bss.options.preprocessed_file) {
        if (_dup(1) == -1 || !(c1_bss.preprocessed_stream =
            freopen(c1_bss.options.preprocessed_file, "wt+", stdout)))
            report_response_error(83, 340, c1_bss.options.preprocessed_file);
    }
    c1_bss.preprocessor_output = stdout;
    if (!c1_data.options.preprocess_no_lines)
        begin_line_output();
    arena = &c1_bss.arenas[1];
    memory = arena->cursor;
    arena->cursor += sizeof(CompilerLexer);
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    lexer = memory ? new(memory) CompilerLexer(c1_bss.current_context.object,
                                               c1_bss.current_context.value) : NULL;
    c1_bss.current_context.Push(lexer, 1, 0, 0, &c1_bss.current_context);
    if (!c1_bss.lexer_backend) {
        backend = (CompilerLexerBackend *)::operator new(sizeof(CompilerLexerBackend));
        c1_bss.lexer_backend = backend;
        if (backend) {
            backend->enabled = c1_bss.browser_filename != NULL;
            backend->backend = c1_data.backend;
        }
    }
    if (!c1_data.options.pch_use)
        begin_source_input();
}

extern "C" void __cdecl normalize_frontend_options(void)
{
    c1_bss.processor_instruction_mask = 1U << (c1_data.options.processor & 31);
    if (c1_data.options.minimal_rebuild)
        c1_bss.processor_instruction_mask |= 0x40;
}
extern "C" void __cdecl format_processor_switch(char *buffer, const char *format, unsigned size, int processor) { __asm { int 3 } }
extern "C" int __cdecl prepare_pch_input(const char *filename) { __asm { int 3 } return 0; }
extern "C" void __cdecl read_initial_input(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_line_tables(void) { }
extern "C" void __cdecl initialize_type_table(void)
{
    CompilerArena *arena = &c1_bss.arenas[0];
    BYTE *storage = arena->cursor;
    arena->cursor += 8192;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    c1_data.type_buckets = (CompilerTypeEntry **)storage;
    storage = arena->cursor;
    arena->cursor += 4096;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    c1_data.type_shape_buckets = (CompilerTypeShapeEntry **)storage;
    storage = arena->cursor;
    arena->cursor += 4096;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    c1_data.type_list_buckets = (CompilerTypeListEntry **)storage;
}
extern "C" CompilerType *__cdecl intern_type(CompilerType *description)
{
    unsigned hash = (((WORD)(unsigned)description->shape >> 2) +
                     ((WORD)description->auxiliary >> 2) +
                     description->attributes + description->flags) & 2047;
    CompilerTypeEntry **bucket = &c1_data.type_buckets[hash];
    CompilerTypeEntry *entry;
    CompilerPool *pool = &c1_bss.pools[0];
    for (entry = *bucket; entry; entry = entry->next) {
        CompilerType *type = &entry->type;
        if (type->shape == description->shape &&
            type->auxiliary == description->auxiliary &&
            type->flags == description->flags &&
            type->attributes == description->attributes)
            return type;
    }
    if (pool->remaining < 20)
        pool->Refill(20);
    entry = (CompilerTypeEntry *)pool->cursor;
    pool->cursor += 20;
    pool->remaining -= 20;
    entry->next = *bucket;
    *bucket = entry;
    entry->type = *description;
    if (c1_data.options.debug_info)
        entry->type.debug_index = 0;
    return &entry->type;
}
extern "C" void __cdecl initialize_global_scope(void)
{
    c1_bss.saved_symbol_arenas = NULL;
    c1_bss.symbol_arena_kind = 2;
}
extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
struct BuiltinNameDescription { CompilerNameEntry **slot; const char *name; };
/* The original table at RVA60020 ends with a NULL destination slot. */
static const BuiltinNameDescription builtin_names[] = {
    { &c1_data.builtin_names.defined_operator, "defined" },
    { &c1_data.builtin_names.formal_parameter, "__formal" },
    { &c1_data.builtin_names.ellipsis, "..." },
    { &c1_data.builtin_names.abstract_declarator, "abstract declarator" },
    { &c1_data.builtin_names.main_entry, "main" },
    { &c1_data.builtin_names.winmain_entry, "WinMain" },
    { &c1_data.builtin_names.dllmain_entry, "DllMain" },
    { &c1_data.builtin_names.alignment_member, ".alignment member." },
    { &c1_data.builtin_names.unnamed, "__unnamed" },
    { &c1_data.builtin_names.dollar, "$" },
    { &c1_data.builtin_names.directive_section, ".drectve" },
    { &c1_data.builtin_names.local_static_guard, ".local static guard." },
    { &c1_data.builtin_names.setjmp_name, "_setjmp" },
    { &c1_data.builtin_names.catch_name, "Catch" },
    { &c1_data.builtin_names.local_size, "__LOCAL_SIZE" },
    { &c1_data.builtin_names.line_variable, "__LINE__Var" },
    { NULL, NULL }
};
extern "C" void __cdecl initialize_builtin_symbols(void)
{
    const BuiltinNameDescription *description;
    for (description = builtin_names; description->slot; ++description)
        *description->slot = find_or_create_identifier(description->name);
}
extern "C" CompilerType *__cdecl make_function_type(unsigned attributes, CompilerType *base)
{
    CompilerDeclarator declarator;
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerParameterDescription *parameters;
    CompilerDeclarationAttributes applied;
    declarator.Initialize(base, &c1_bss.declaration_position);
    parameters = (CompilerParameterDescription *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    if (parameters) {
        parameters->kind = 1;
        parameters->scope = NULL;
        parameters->value = 0;
    }
    declarator.AddFunction(parameters);
    /* Original writes only a WORD here, leaving the high WORD of the first
     * attributes DWORD uninitialized. Do not reproduce those incidental bytes. */
    applied.flags = (WORD)attributes;
    applied.extra = 0;
    declarator.ApplyAttributes(&applied);
    return declarator.Resolve();
}
extern "C" CompilerType *__cdecl make_tagged_type(unsigned flags) { __asm { int 3 } return NULL; }
extern "C" void __cdecl initialize_intermediate_output(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_translation_state(void) { __asm { int 3 } }
extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions) { __asm { int 3 } }
extern "C" void __cdecl begin_line_output(void) { __asm { int 3 } }
extern "C" void __cdecl begin_source_input(void) { __asm { int 3 } }

CompilerTranslationUnit *RECON_THISCALL CompilerTranslationUnit::Initialize(void)
{
    CompilerPool *pool = &c1_bss.pools[7];
    void *memory;
    CompilerSymbolTable *table;
    unsigned bytes;
    depth = 0;
    lookup_mode = 0;
    retired_scopes = NULL;
    symbols = NULL;
    type_aliases = NULL;
    if (pool->remaining < 24)
        pool->Refill(24);
    memory = pool->cursor;
    pool->cursor += 24;
    pool->remaining -= 24;
    table = memory ? new(memory) CompilerSymbolTable : NULL;
    root = table;
    bytes = table->bucket_mask * 4 + 4;
    if (pool->remaining < bytes)
        pool->Refill(bytes);
    table->buckets = (CompilerIrSymbol **)pool->cursor;
    pool->cursor += bytes;
    pool->remaining -= bytes;
    current = root;
    scopes[0] = root;
    return this;
}
int RECON_THISCALL CompilerSymbolTable::HasBuckets(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerSymbolTable::Insert(CompilerIrSymbol *symbol) { __asm { int 3 } }
void *RECON_THISCALL CompilerSymbolTable::Find(CompilerNameEntry *name) { __asm { int 3 } return NULL; }
void *RECON_THISCALL CompilerSymbolTable::Lookup(CompilerNameEntry *name) { __asm { int 3 } return NULL; }
void *RECON_THISCALL CompilerSymbolTable::FindOrdinary(CompilerNameEntry *name) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerSymbolTable::FindNested(CompilerNameEntry *name, void **result) { __asm { int 3 } }
CompilerIrSymbol **RECON_THISCALL CompilerSymbolTable::FindLink(CompilerIrSymbol *symbol) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerSymbolTable::Serialize(void) { __asm { int 3 } }
void RECON_THISCALL CompilerSymbolTable::Check(void) { __asm { int 3 } }
void RECON_THISCALL CompilerSymbolTable::Link(CompilerIrSymbol *symbol) { __asm { int 3 } }
extern "C" CompilerIrSymbol *__cdecl make_abstract_declarator(void);
CompilerDeclarator *RECON_THISCALL CompilerDeclarator::Initialize(CompilerType *base,
    const CompilerSourcePosition *position)
{
    /* The actual initializer clears all48 object bytes, including the fields
     * whose meanings remain unrecovered. This is not an image-state copy. */
    memset(this, 0, sizeof(*this));
    symbol = make_abstract_declarator();
    this->position = *position;
    base_type = base;
    return this;
}
extern "C" CompilerIrSymbol *__cdecl make_abstract_declarator(void)
{
    unsigned arena_index = (c1_bss.declaration_nesting <= 0) + 2;
    CompilerArena *arena = &c1_bss.arenas[arena_index];
    CompilerIrSymbol *symbol = NULL;
    CompilerNameEntry *name = c1_data.builtin_names.abstract_declarator;
    BYTE *memory;
#ifdef RECON_NATIVE_TEST
    /* Other original cache indexes/record sizes are valid but unmodeled. */
    if (c1_data.abstract_symbol_bytes != 52)
        recon_unsupported("make_abstract_declarator: unmodeled record size");
#endif
    if (!c1_bss.declaration_nesting) {
        CompilerSymbolBinding *binding;
#ifdef RECON_NATIVE_TEST
        if (c1_data.abstract_symbol_cache_index != 9)
            recon_unsupported("make_abstract_declarator: unmodeled cache head");
#endif
        /* Typed projection of free_heads[9], not a claim about other heads. */
        binding = c1_data.abstract_symbol_free;
        if (binding) {
            c1_data.abstract_symbol_free = binding->next;
            symbol = (CompilerIrSymbol *)((BYTE *)binding - 8);
            binding->name = NULL;
            binding->next = NULL;
            memset(symbol, 0, c1_data.abstract_symbol_bytes);
            binding->name = name;
            symbol->kind = 13;
        }
    }
    if (!symbol) {
        memory = arena->cursor;
        arena->cursor += (c1_data.abstract_symbol_bytes + 3) & ~3U;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        symbol = memory ? ((CompilerIrSymbol *)memory)->Initialize(13, name, arena_index) : NULL;
    }
    memory = arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    symbol->abstract_type = (CompilerAbstractType *)memory;
    return symbol;
}
CompilerIrSymbol *RECON_THISCALL CompilerIrSymbol::Initialize(unsigned kind,
    CompilerNameEntry *name, unsigned arena)
{
    this->kind = (BYTE)kind;
    binding.next = NULL;
    binding.name = name;
    if ((BYTE)kind == 4)
        kind4_context = NULL;
    return this;
}
extern "C" CompilerTypeShape *__cdecl create_type_shape(unsigned, unsigned, const unsigned *);
void RECON_THISCALL CompilerDeclarator::AddFunction(CompilerParameterDescription *parameters)
{
    int first = shape == NULL;
    unsigned value = 0;
    CompilerTypeShape *function;
    flags |= 0x10;
    parameters->Normalize();
    function = create_type_shape(4, 0, &value);
    function->SetParameters(parameters->value, parameters->kind);
    if (first && (attributes.flags & 0x70)) {
        function->attributes |= attributes.flags & 0x70;
        attributes.flags &= ~0x70U;
    }
    AppendShape(function);
    if (!last_shape) {
        last_shape = function;
        shape = function;
    } else {
        last_shape->base = function;
        last_shape = function;
    }
    if (first) {
        this->parameters = parameters;
        flags |= 8;
    }
}
extern "C" unsigned __cdecl classify_parameter_list(const void *);
void RECON_THISCALL CompilerParameterDescription::Normalize(void)
{
    if (scope)
        c1_data.translation_unit->EndParameterScope();
    if (kind == 6)
        kind = classify_parameter_list((const void *)value);
}
CompilerIrSymbol **RECON_THISCALL CompilerTranslationUnit::EndParameterScope(void)
{ __asm { int 3 } return NULL; }
extern "C" unsigned __cdecl classify_parameter_list(const void *value)
{ __asm { int 3 } return 0; }
extern "C" CompilerTypeShape *__cdecl create_type_shape(unsigned kind, unsigned attributes,
    const unsigned *value)
{
    CompilerArena *arena = &c1_bss.arenas[3];
    CompilerTypeShape *shape = (CompilerTypeShape *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    shape->flags = (WORD)kind;
    shape->attributes = (WORD)attributes;
    shape->value = *value;
    /* The original leaves the base/link field for the caller to establish. */
    return shape;
}
extern "C" struct CompilerTypeList *__cdecl intern_function_parameters(const void *,
    CompilerTypeShape *, unsigned);
void RECON_THISCALL CompilerTypeShape::SetParameters(unsigned value, unsigned kind)
{
    if (kind == 2)
        value = 0;
    CompilerTypeList *parameters = intern_function_parameters((const void *)value, this, kind);
    flags |= 0x400;
    this->parameters = parameters;
}
extern "C" CompilerTypeList *__cdecl get_empty_type_list(void);
extern "C" CompilerTypeList *__cdecl get_void_type_list(void);
extern "C" unsigned __cdecl hash_type_list(const CompilerTypeList *);
extern "C" void __cdecl compiler_warning(int, ...);
struct CompilerParameterSequence {
    BYTE unmodeled_00[4];
    CompilerSymbolBinding *first;
};
extern "C" struct CompilerTypeList *__cdecl intern_function_parameters(const void *value,
    CompilerTypeShape *function, unsigned kind)
{
    const CompilerParameterSequence *sequence = (const CompilerParameterSequence *)value;
    CompilerSymbolBinding *parameter;
    CompilerIrSymbol *symbol;
    CompilerTypeList *temporary;
    CompilerTypeListEntry *entry, *previous, **bucket, *created;
    CompilerArena *arena;
    BYTE *memory;
    CompilerType **next_type;
    unsigned bytes;
    int count = 0, has_void = 0;
    if (!sequence) {
        function->flags |= 0x800;
        return get_empty_type_list();
    }
    if (kind != 5) {
        for (parameter = sequence->first; parameter; parameter = parameter->next) {
            symbol = (CompilerIrSymbol *)((BYTE *)parameter - 8);
            if (symbol->kind == 11 || symbol->kind == 12) {
                if (symbol->parameter_type == c1_data.builtin_types.type_0080)
                    has_void = 1;
                ++count;
                if (!symbol->parameter_type)
                    function->flags |= 0x8000;
            }
        }
        if (!has_void) {
            if (!count) {
                function->flags |= 0x800;
                return get_empty_type_list();
            }
            bytes = count * 4;
            arena = &c1_bss.arenas[3];
            temporary = (CompilerTypeList *)arena->cursor;
            arena->cursor += (bytes + 11) & ~3U;
            if (arena->cursor > arena->committed_limit)
                arena->CommitExtent();
            temporary->count = count;
            next_type = temporary->types;
            for (parameter = sequence->first; parameter; parameter = parameter->next) {
                symbol = (CompilerIrSymbol *)((BYTE *)parameter - 8);
                if (symbol->kind == 11 || symbol->kind == 12)
                    *next_type++ = symbol->parameter_type;
            }
            bucket = &c1_data.type_list_buckets[hash_type_list(temporary) & 1023];
            previous = NULL;
            for (entry = *bucket; entry; previous = entry, entry = entry->next) {
                if (entry->list.count < count) continue;
                if (entry->list.count > count) break;
                if (!memcmp(entry->list.types, temporary->types, bytes))
                    return &entry->list;
            }
            arena = &c1_bss.arenas[0];
            memory = arena->cursor;
            arena->cursor += (bytes + 15) & ~3U;
            if (arena->cursor > arena->committed_limit)
                arena->CommitExtent();
            created = (CompilerTypeListEntry *)memory;
            created->next = entry;
            if (previous)
                previous->next = created;
            else
                *bucket = created;
            /* Original also transfers the opaque DWORD at+4. No consumer
             * here interprets it; semantic uses remain a later frontier. */
            memcpy(&created->list, temporary, bytes + 8);
            return &created->list;
        }
        symbol = (CompilerIrSymbol *)((BYTE *)sequence->first - 8);
        if (count != 1 && !(symbol->scope_bits & 1)) {
            compiler_warning(860);
            symbol->scope_bits |= 1;
        }
    }
    function->flags |= 0x4000;
    symbol = (CompilerIrSymbol *)((BYTE *)sequence->first - 8);
    symbol->flags |= 2;
    return get_void_type_list();
}
extern "C" CompilerTypeList *__cdecl get_empty_type_list(void)
{
    CompilerArena *arena;
    CompilerTypeList *list;
    if (c1_data.empty_type_list)
        return c1_data.empty_type_list;
    arena = &c1_bss.arenas[0];
    list = (CompilerTypeList *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    c1_data.empty_type_list = list;
    list->count = 0;
    memset(list->unmodeled_04, 0, 4);
    /* Its unused first element is not written by the original getter. */
    return c1_data.empty_type_list;
}
extern "C" CompilerTypeList *__cdecl get_void_type_list(void) { __asm { int 3 } return NULL; }
extern "C" unsigned __cdecl hash_type_list(const CompilerTypeList *list) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerDeclarator::AppendShape(CompilerTypeShape *next)
{
    shape_flags |= 1;
    if (!shape)
        return;
    switch (last_shape->flags & 7) {
    case 1:
        last_shape->flags = (next->flags & 0xc803) | 3;
        break;
    case 2:
        compiler_warning(92);
        last_shape->value = 0;
        last_shape->flags = 1;
        break;
    case 4: {
        /* Only the low WORDs are read. Zero the original unwritten upper
         * WORDs of these temporary attribute descriptors. */
        CompilerDeclarationAttributes first = { 0, 0 }, second = { 0, 0 };
        compiler_warning(91);
        MergeQualifiers(&first, &second);
        last_shape->flags = 3;
        break;
    }
    }
    if (last_shape->attributes & 3) {
        compiler_warning(212, symbol);
        flags |= 1;
        last_shape->attributes &= ~3;
    }
}
void RECON_THISCALL CompilerDeclarator::MergeQualifiers(const CompilerDeclarationAttributes *first,
    const CompilerDeclarationAttributes *second) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarator::ApplyAttributes(const CompilerDeclarationAttributes *attributes)
{
    if (attributes->flags & 1) {
        compiler_warning(493);
        flags |= 1;
    }
    this->attributes = *attributes;
}
extern "C" CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *shape);
CompilerType *RECON_THISCALL CompilerDeclarator::Resolve(void)
{
    CompilerType *result;
    if ((WORD)attributes.flags == 0 && !shape) {
        if (base_type->shape)
            base_type->shape = intern_type_shape(base_type->shape);
        base_type = intern_type(base_type);
        flags |= 4;
        result = base_type;
    } else {
        FinalizeShape();
        if (shape_flags & 1) {
            CompilerTypeShape *stop = last_shape->base;
            CompilerTypeShape *previous = NULL, *part;
            if (flags & 8)
                flags |= 2;
            for (part = shape; part != stop; previous = part, part = part->base) {
                WORD convention = part->attributes & 0x70;
                if (convention) {
                    int warning = 0;
                    part->attributes &= ~0x70;
                    switch (part->flags & 7) {
                    case 1:
                        warning = 165;
                        break;
                    case 3:
                        if (part->base != stop)
                            convention = ApplyCallingConvention(part->base, convention);
                        break;
                    case 4:
                        convention = ApplyCallingConvention(part, convention);
                        if (part != shape)
                            previous->attributes |= convention;
                        break;
                    default:
                        continue;
                    }
                    if (part == shape && warning) {
                        compiler_warning(warning, 120);
                        flags |= 1;
                        continue;
                    }
                    part->attributes |= convention;
                } else if ((part->flags & 7) == 4) {
                    if (part == shape) {
                        convention = (WORD)attributes.flags & 0x70;
                        if (convention)
                            attributes.flags = (attributes.flags & ~0xffffU) |
                                ((WORD)attributes.flags & ~0x70);
                        else {
                            flags &= ~2;
                            convention = ResolveFunctionAttributes(part);
                        }
                    } else {
                        convention = previous->attributes & 0x70;
                        if (!convention) {
                            convention = ResolveFunctionAttributes(part);
                            previous->attributes |= convention;
                        }
                    }
                    part->attributes |= convention;
                }
            }
        }
        ApplyBaseAttributes();
        CompilerTypeShape *stop = last_shape ? last_shape->base : NULL;
        result = BuildType(shape->InternUntil(stop));
    }
    resolved_type = result;
    if (!result) {
        flags |= 1;
        resolved_type = c1_data.builtin_types.type_0063;
    }
    return result;
}
extern "C" CompilerTypeShape *__cdecl copy_type_shape_chain(CompilerTypeShape *);
/* The tagged-type consumer establishes only this flag byte. Its producer,
 * allocation extent and other fields remain pending. */
struct CompilerTagState { BYTE unmodeled_00[4]; BYTE flags; };
void RECON_THISCALL CompilerDeclarator::FinalizeShape(void)
{
    if (base_type->shape) {
        CompilerTypeShape *copy = copy_type_shape_chain(base_type->shape);
        switch (copy->flags & 7) {
        case 1: case 3:
            break;
        case 2:
            AppendArray(copy);
            break;
        case 4:
            AppendShape(copy);
            break;
        default:
            return;
        }
        if (last_shape)
            last_shape->base = copy;
        else
            shape = copy;
        last_shape = copy;
        return;
    }
    if (!shape)
        return;
    if ((base_type->flags & 0x2fcf) == 0x80 &&
        (last_shape->flags & 7) == 2) {
        compiler_warning(182, symbol);
        flags |= 1;
        base_type = c1_data.builtin_types.type_0063;
    }
    if ((last_shape->flags & 7) == 2 && !base_type->shape &&
        (base_type->flags & 0x400)) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("FinalizeShape: tagged-type owner is not yet modeled");
#endif
        CompilerTagState *tag = (CompilerTagState *)base_type->auxiliary;
        if (tag->flags & 1) {
            compiler_warning(233, symbol);
            flags |= 1;
        }
    }
}
void RECON_THISCALL CompilerDeclarator::AppendArray(CompilerTypeShape *shape)
{ __asm { int 3 } }
extern "C" CompilerTypeShape *__cdecl copy_type_shape_chain(CompilerTypeShape *shape)
{ __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerDeclarator::ApplyBaseAttributes(void) { __asm { int 3 } }
WORD RECON_THISCALL CompilerDeclarator::ApplyCallingConvention(CompilerTypeShape *shape,
    unsigned convention) { __asm { int 3 } return 0; }
extern "C" void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
WORD RECON_THISCALL CompilerDeclarator::ResolveFunctionAttributes(CompilerTypeShape *shape)
{
    if (shape->flags & 0x8000) {
        if (c1_data.options.fastcall_calls || c1_data.options.pascal_calls)
            report_language_diagnostic(2, 96, 37, 106);
        return 0x10;
    }
    if (c1_data.options.fastcall_calls)
        return 0x40;
    if (c1_data.options.stdcall_calls)
        return 0x60;
    return c1_data.options.pascal_calls ? 0x20 : 0x10;
}
CompilerType *RECON_THISCALL CompilerDeclarator::BuildType(CompilerTypeShape *shape)
{ __asm { int 3 } return NULL; }
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::InternUntil(CompilerTypeShape *stop)
{ __asm { int 3 } return NULL; }
extern "C" CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *shape)
{ __asm { int 3 } return NULL; }
