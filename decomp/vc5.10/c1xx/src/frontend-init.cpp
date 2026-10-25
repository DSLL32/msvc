#include <io.h>
#include <new.h>
#include <stddef.h>
#include <string.h>
#include "frontend-init.h"
#include "pointer-array.h"
#include "packing.h"
#pragma intrinsic(memset, memcmp)
#include <recon.h>

/* Preserve the observed direct FILE-array import, not SDK __p__iob. */
#if defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
#undef _iob
#undef stdout
extern "C" __declspec(dllimport) FILE _iob[];
#define stdout (&_iob[1])
#endif

extern "C" {
int __cdecl create_input(int, CompilerInput **, CompilerTextInput **);
void __cdecl normalize_frontend_options(void);
void __cdecl format_processor_switch(char *, const char *, unsigned, int);
void __cdecl begin_browser_output(const char *);
CompilerBackend *__cdecl acquire_initial_backend(void);
int __cdecl prepare_pch_input(const char *);
void __cdecl read_initial_input(void);
void __cdecl initialize_preprocessor(void);
void __cdecl initialize_keyword_table(void);
void __cdecl initialize_language_keywords(void);
void __cdecl initialize_line_tables(void);
void __cdecl initialize_frontend_tables(void);
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
void __cdecl initialize_first_token(void);
void __cdecl initialize_symbol_groups(void);
void __cdecl initialize_language_symbols(void);
void __cdecl initialize_scope_symbols(void);
CompilerSymbol *__cdecl lookup_builtin(CompilerNameEntry *, CompilerDeclarator *);
void __cdecl declare_builtin(CompilerDeclaration *, CompilerSymbol *);
int __cdecl report_response_error(int, ...);
void __cdecl compiler_warning(int, ...);
void __cdecl report_language_diagnostic(unsigned, unsigned, ...);
CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *);
CompilerTypeShape *__cdecl copy_type_shape(CompilerTypeShape *);
CompilerSymbol *__cdecl find_declarator_type(CompilerSymbol *, unsigned);
CompilerSymbol *__cdecl current_declarator_context(void);
int __cdecl check_member_declaration(CompilerSymbol *);
void __cdecl diagnose_member_declaration(CompilerSymbol *);
CompilerSymbol *__cdecl recycle_symbol(unsigned, CompilerNameEntry *, unsigned);
CompilerHashTable *__cdecl create_hash_table(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
CompilerSymbol *__cdecl make_abstract_declarator(void);
CompilerTypeShape *__cdecl create_type_shape(unsigned, unsigned, const unsigned *);
CompilerType *__cdecl qualify_type(CompilerSymbol *, unsigned, WORD);
CompilerParameterLink *__cdecl link_parameter_symbols(struct CompilerParameterContext *);
CompilerFunctionType *__cdecl create_function_parameters(CompilerParameterSet *, WORD *, CompilerParameterLink *, CompilerType *);
CompilerParameterTypes *__cdecl empty_parameter_types(void);
CompilerParameterTypes *__cdecl collect_parameter_types(unsigned, CompilerParameterSet *);
CompilerFunctionType *__cdecl intern_function_type(CompilerFunctionType *);
}

extern "C" void __cdecl initialize_frontend(void)
{
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerType *description;
    CompilerBuiltinTypes *types = &frontend_data.builtin_types;
    CompilerLexer *lexer;
    CompilerLexerBackend *backend;
    CompilerDeclarator lookup;
    CompilerDeclaration declaration;
    void *memory;
    unsigned alignment;
    int record;
    char processor_switch[20];

    record = frontend_data.options.incremental_compile &&
        !frontend_bss.options.preprocess_stdout && !frontend_bss.options.prototypes_only;
    if (!create_input(record, &frontend_bss.input, &frontend_bss.text_input))
        report_compiler_diagnostic(73, ".\\main.c", 178);
    SetHandleCount(30);
    frontend_data.options.preprocess_no_lines = frontend_data.options.preprocess_no_lines ||
        !frontend_bss.options.preprocess_stdout;
    alignment = frontend_data.options.packing_alignment;
    if (alignment == (unsigned)-1)
        alignment = 8;
    frontend_data.options.packing_stack->top->alignment = alignment;
    frontend_data.normalized_stack_checks = frontend_data.options.stack_checks;
    frontend_data.normalized_oq = frontend_data.options.option_oq;
    frontend_data.normalized_pool_strings = frontend_data.options.pool_strings;
    frontend_data.normalized_readonly_strings = frontend_data.options.readonly_strings;
    if ((unsigned)frontend_data.options.processor > 6) {
        format_processor_switch(processor_switch, "-G%d", 20, frontend_data.options.processor);
        report_compiler_diagnostic(7, processor_switch, "c1");
    } else if ((unsigned)frontend_data.options.processor > 4) {
        frontend_data.options.processor = 4;
    }
    normalize_frontend_options();
    if (frontend_bss.options.preprocess_stdout || frontend_bss.options.prototypes_only) {
        frontend_bss.options.browser_file = NULL;
        frontend_bss.options.browser_local_file = NULL;
        frontend_data.browser_full = 0;
        frontend_data.browser_enabled = 0;
    } else {
        if (frontend_bss.options.browser_file) {
            frontend_bss.options.browser_local_file = frontend_bss.options.browser_file;
            frontend_data.browser_full = 1;
        }
        frontend_data.browser_enabled = frontend_bss.options.browser_local_file != NULL;
    }
    frontend_bss.browser_filename = frontend_bss.options.browser_local_file;
    begin_browser_output(frontend_bss.options.browser_local_file);
    frontend_data.backend = acquire_initial_backend();
    if (frontend_data.options.pch_no_preprocess) {
        if (frontend_data.options.pch_create_file ||
            (frontend_data.options.pch_file && !frontend_data.options.pch_use)) {
            if (prepare_pch_input(frontend_data.options.pch_create_file)) {
                frontend_bss.pch_replayed = 1;
                frontend_bss.pch_input_ready = frontend_data.options.pch_use;
                frontend_bss.pch_input_file = frontend_data.options.pch_use_file;
                if (frontend_data.options.pch_use)
                    frontend_data.options.pch_file = NULL;
                frontend_data.options.pch_use = 1;
                frontend_data.options.pch_use_file = frontend_data.options.pch_create_file;
                frontend_data.options.pch_create = 0;
                frontend_data.options.pch_create_file = NULL;
            } else {
                frontend_bss.pch_file_failed = 1;
                read_initial_input();
            }
        } else {
            frontend_data.options.pch_no_preprocess = 0;
        }
    }
    frontend_data.saved_pch_mode = frontend_data.options.pch_create;
    if (frontend_data.options.debug_info && !frontend_data.options.pch_debug) {
        frontend_data.pch_separate_debug = 1;
        if (!frontend_data.options.pch_create)
            initialize_preprocessor();
    } else {
        frontend_data.pch_separate_debug = 0;
        initialize_preprocessor();
    }
    initialize_keyword_table();
    initialize_language_keywords();
    initialize_line_tables();
    initialize_frontend_tables();
    if (!frontend_bss.options.preprocess_stdout && !frontend_bss.options.prototypes_only) {
        if (frontend_data.option_c9il) {
            open_expression_output();
            frontend_bss.output_buffers[1].Open(frontend_data.options.il_prefix, "sy", "w+b");
            frontend_bss.output_buffers[2].Open(frontend_data.options.il_prefix, "gl", "w+b");
            frontend_bss.output_buffers[4].Open(frontend_data.options.il_prefix, "in", "w+b");
            if (frontend_data.options.debug_info)
                frontend_bss.output_buffers[0].Open(frontend_data.options.il_prefix, "db", "w+b");
        } else {
            frontend_bss.output_kinds[0] = 3; /* RVA54c68 */
            frontend_bss.output_kinds[1] = 1; /* RVA53fe8 */
            frontend_bss.output_kinds[2] = 4; /* RVA54f18 */
            frontend_bss.output_kinds[3] = 2; /* RVA54a50 */
            frontend_bss.output_kinds[4] = 1; /* RVA547e8 */
            frontend_bss.output_kinds[5] = 5; /* RVA53eb8 */
        }
    }
    initialize_type_table();
    arena = &frontend_bss.arenas[5];
    description = (CompilerType *)arena->cursor;
    arena->cursor += 16; /* actual allocation; unmodeled tail has no fields */
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    description->attributes = 0;
    description->shape = NULL;
    description->auxiliary = 0;
    /* Natural padding and debug_cache are not explicitly initialized here;
     * this descriptor occupies the original zero-backed arena allocation. */
    description->flags = 0;
    types->type_0000 = intern_type(description);
    description->flags = 0x4041;
    types->type_4041_first = intern_type(description);
    description->flags = 0x63;
    types->type_0063 = intern_type(description);
    description->flags = 0x4063;
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
    description->flags = 0x60;
    types->type_0060 = intern_type(description);
    description->flags = 0x13;
    types->type_0013 = intern_type(description);
    description->flags = 0x4063;
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
    description->flags = 0x63;
    types->type_0063_q0100 = intern_type(description);
    description->attributes = 0;
    initialize_global_scope();
    initialize_builtin_symbols();
    pool = &frontend_bss.pools[8];
    if (pool->remaining < 572)
        pool->Refill(572);
    memory = pool->cursor;
    pool->cursor += 572;
    pool->remaining -= 572;
    /* The pending initializer owns the unrecovered translation-unit layout. */
    frontend_data.translation_unit = memory ?
        ((CompilerTranslationUnit *)memory)->Initialize() : NULL;
    if (frontend_data.options.incremental_rebuild || frontend_data.options.dependency_output) {
        arena = &frontend_bss.arenas[1];
        memory = arena->cursor;
        arena->cursor += 16; /* original allocation, only records is modeled */
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        if (memory) {
            frontend_data.dependency = (CompilerDependency *)memory;
            frontend_data.dependency->records = NULL;
        } else {
            frontend_data.dependency = NULL;
        }
    }
    if (!frontend_bss.options.preprocess_stdout) {
        initialize_symbol_groups();
        types->implicit_int_function = make_function_type(0, types->type_0063);
        types->tagged_0080 = make_tagged_type(0x80);
        if (!frontend_bss.options.prototypes_only)
            initialize_intermediate_output();
        initialize_translation_state();
    }
    define_command_line_macros(frontend_bss.options.defines);
    if (frontend_bss.options.preprocessed_file) {
        if (_dup(1) == -1 || !(frontend_bss.preprocessed_stream =
            freopen(frontend_bss.options.preprocessed_file, "wt+", stdout)))
            report_response_error(83, 340, frontend_bss.options.preprocessed_file);
    }
    frontend_bss.preprocessor_output = stdout;
    if (!frontend_data.options.preprocess_no_lines)
        begin_line_output();
    if (!frontend_bss.options.preprocess_stdout) {
        initialize_first_token();
        if (!frontend_data.options.pch_use) {
            initialize_language_symbols();
            initialize_scope_symbols();
            if (frontend_data.options.language_extensions && !frontend_data.options.no_bool) {
                /* Only these fields of the zeroed 40-byte original record
                 * are modeled. Pending consumers must recover any others. */
                declaration.flags = 7;
                declaration.type = types->type_0060;
                lookup.Initialize(types->type_0060, &frontend_bss.declaration_context);
                lookup.Resolve();
                declare_builtin(&declaration,
                    lookup_builtin(frontend_data.bool_identifier, &lookup));
            }
        }
    }
    arena = &frontend_bss.arenas[1];
    memory = arena->cursor;
    arena->cursor += 88; /* original allocation; unknown tail stays unmodeled */
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    lexer = memory ? new(memory) CompilerLexer(frontend_bss.saved_context_stack.origin,
                                               frontend_bss.saved_context_stack.position) : NULL;
    frontend_bss.saved_context_stack.Push(lexer, 1, 0, 0, &frontend_bss.saved_context_stack);
    if (!frontend_bss.lexer_backend) {
        backend = (CompilerLexerBackend *)::operator new(12);
        frontend_bss.lexer_backend = backend;
        if (backend) {
            backend->enabled = frontend_bss.browser_filename != NULL;
            backend->backend = frontend_data.backend;
        }
    }
    if (!frontend_data.options.pch_use)
        begin_source_input();
}

extern "C" void __cdecl normalize_frontend_options(void)
{
    frontend_bss.processor_instruction_mask = 1U << (frontend_data.options.processor & 31);
    if (frontend_data.options.minimal_rebuild)
        frontend_bss.processor_instruction_mask |= 0x40;
}
extern "C" void __cdecl format_processor_switch(char *buffer, const char *format, unsigned size, int processor) { __asm { int 3 } }
extern "C" int __cdecl prepare_pch_input(const char *filename) { __asm { int 3 } return 0; }
extern "C" void __cdecl read_initial_input(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_keyword_table(void)
{
    CompilerArena *arena = &frontend_bss.arenas[0];
    if (!frontend_data.keyword_table) {
        BYTE *table = arena->cursor;
        arena->cursor += 8192;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        frontend_data.keyword_table = (CompilerKeywordTable *)table;
    }
}
extern "C" void __cdecl initialize_line_tables(void) {}
extern "C" void __cdecl initialize_frontend_tables(void)
{
    CompilerPointerArray *array = new_pointer_array(1600);
    frontend_data.frontend_pointer_table = array;
    if (array->count == array->capacity)
        array = grow_pointer_array(array);
    array->items[array->count] = NULL;
    ++array->count;
    frontend_data.frontend_pointer_table = array;
}
extern "C" void __cdecl initialize_type_table(void)
{
    CompilerArena *arena = &frontend_bss.arenas[0];
    BYTE *table = arena->cursor;
    arena->cursor += 8192;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    frontend_data.type_tables.types = (CompilerTypeHash *)table;
    table = arena->cursor;
    arena->cursor += 4096;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    frontend_data.type_tables.mapped = (CompilerTypeMap *)table;
    table = arena->cursor;
    arena->cursor += 4096;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    frontend_data.type_tables.composite = (CompilerCompositeTypeHash *)table;
    table = arena->cursor;
    arena->cursor += 2048;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    frontend_data.type_tables.sequences = (CompilerTypeSequenceHash *)table;
}
extern "C" CompilerType *__cdecl intern_type(CompilerType *description)
{
    unsigned hash = (((WORD)(unsigned)description->shape >> 2) +
        description->attributes + description->flags + description->auxiliary) & 0x7ff;
    CompilerInternedType **bucket = &frontend_data.type_tables.types->buckets[hash];
    CompilerInternedType *entry;
    CompilerPool *pool = &frontend_bss.pools[0];
    for (entry = *bucket; entry; entry = entry->next) {
        CompilerType *type = &entry->type;
        if (type->shape == description->shape && type->auxiliary == description->auxiliary &&
            type->flags == description->flags && type->attributes == description->attributes)
            return type;
    }
    if (pool->remaining < 20) pool->Refill(20);
    entry = (CompilerInternedType *)pool->cursor;
    pool->cursor += 20;
    pool->remaining -= 20;
    entry->next = *bucket;
    *bucket = entry;
    /* A typed value transfer preserves the modeled members. No equivalence
     * or semantic read is claimed for the original two padding bytes. */
    entry->type = *description;
    if (frontend_data.options.debug_info) entry->type.debug_cache = 0;
    return &entry->type;
}
extern "C" void __cdecl initialize_global_scope(void)
{
    frontend_bss.saved_symbol_arenas = NULL;
    frontend_bss.symbol_arena_kind = 4;
}
extern "C" CompilerNameEntry *__cdecl find_or_create_identifier(const char *);
struct BuiltinNameDescription { CompilerNameEntry **destination; const char *text; };
static const BuiltinNameDescription builtin_names[] = {
    { &frontend_data.builtin_names.return_udt, "__$ReturnUdt" }, /* original 0xcc638 */
    { &frontend_data.builtin_names.this_parameter, "this" }, /* original 0xcc640 */
    { &frontend_data.builtin_names.that_parameter, "__that" }, /* original 0xcc63c */
    { &frontend_data.builtin_names.vftable, "$vftable" }, /* original 0xcc680 */
    { &frontend_data.builtin_names.local_vftable, "$locvftable" }, /* original 0xcc684 */
    { &frontend_data.builtin_names.vbtable, "$vbtable" }, /* original 0xcc64c */
    { &frontend_data.builtin_names.vfptr, "$vfptr" }, /* original 0xcc650 */
    { &frontend_data.builtin_names.vbptr, "$vbptr" }, /* original 0xcc648 */
    { &frontend_data.builtin_names.vdispmap, "$vdispmap" }, /* original 0xcc654 */
    { &frontend_data.builtin_names.initialize_virtual_bases, "$initVBases" }, /* original 0xcc61c */
    { &frontend_data.builtin_names.flags_parameter, "__flags" }, /* original 0xcc618 */
    { &frontend_data.builtin_names.destructor, "__dtor" }, /* original 0xcc614 */
    { &frontend_data.builtin_names.vbase_destructor, "__vbaseDtor" }, /* original 0xcc644 */
    { &frontend_data.builtin_names.deleting_destructor, "__delDtor" }, /* original 0xcc60c */
    { &frontend_data.builtin_names.vector_deleting_destructor, "__vecDelDtor" }, /* original 0xcc664 */
    { &frontend_data.builtin_names.assignment_operator, "=" }, /* original 0xcc460 */
    { &frontend_data.builtin_names.most_derived_parameter, "__most_derived__" }, /* original 0xcc6f0 */
    { &frontend_data.builtin_names.operator_new, "new" }, /* original 0xcc62c */
    { &frontend_data.builtin_names.operator_delete, "delete" }, /* original 0xcc610 */
    { &frontend_data.builtin_names.operator_new_array, "new[]" }, /* original 0xcc630 */
    { &frontend_data.builtin_names.operator_delete_array, "delete[]" }, /* original 0xcc634 */
    { &frontend_data.builtin_names.constructor, "__ctor" }, /* original 0xcc608 */
    { &frontend_data.builtin_names.default_constructor_closure, "__dflt_ctor_closure" }, /* original 0xcc6d8 */
    { &frontend_data.builtin_names.local_vftable_constructor_closure, "__local_vftable_ctor_closure" }, /* original 0xcc6dc */
    { &frontend_data.builtin_names.copy_constructor_closure, "__copy_ctor_closure" }, /* original 0xcc6e0 */
    { &frontend_data.builtin_names.local_static_destructor, ".local static destructor." }, /* original 0xcc620 */
    { &frontend_data.builtin_names.template_static_constructor, ".template static data member ctor function." }, /* original 0xcc624 */
    { &frontend_data.builtin_names.template_static_destructor, ".template static data member dtor function." }, /* original 0xcc628 */
    { &frontend_data.builtin_names.vector_constructor, "__vec_ctor" }, /* original 0xcc668 */
    { &frontend_data.builtin_names.vector_vbase_constructor, "__vec_ctor_vb" }, /* original 0xcc66c */
    { &frontend_data.builtin_names.vector_destructor, "__vec_dtor" }, /* original 0xcc670 */
    { &frontend_data.builtin_names.eh_vector_constructor, "__ehvec_ctor" }, /* original 0xcc674 */
    { &frontend_data.builtin_names.eh_vector_vbase_constructor, "__ehvec_ctor_vb" }, /* original 0xcc678 */
    { &frontend_data.builtin_names.eh_vector_destructor, "__ehvec_dtor" }, /* original 0xcc67c */
    { &frontend_data.builtin_names.new_target, "Target of operator new()" }, /* original 0xcc688 */
    { &frontend_data.builtin_names.atexit_name, "atexit" }, /* original 0xcc68c */
    { &frontend_data.builtin_names.tls_destructor_registration, "__tlregdtor" }, /* original 0xcc690 */
    { &frontend_data.builtin_names.global_namespace, "`global namespace'" }, /* original 0xcc694 */
    { &frontend_data.bool_identifier, "bool" }, /* original 0xcc698 */
    { &frontend_data.builtin_names.defined_operator, "defined" }, /* original 0xcc468 */
    { &frontend_data.builtin_names.formal_parameter, "__formal" }, /* original 0xcc47c */
    { &frontend_data.builtin_names.ellipsis, "..." }, /* original 0xcc478 */
    { &frontend_data.builtin_names.abstract_declarator, "abstract declarator" }, /* original 0xcc464 */
    { &frontend_data.builtin_names.main_entry, "main" }, /* original 0xcc484 */
    { &frontend_data.builtin_names.winmain_entry, "WinMain" }, /* original 0xcc490 */
    { &frontend_data.builtin_names.dllmain_entry, "DllMain" }, /* original 0xcc494 */
    { &frontend_data.builtin_names.alignment_member, ".alignment member." }, /* original 0xcc45c */
    { &frontend_data.builtin_names.unnamed, "__unnamed" }, /* original 0xcc498 */
    { &frontend_data.builtin_names.dollar, "$" }, /* original 0xcc46c */
    { &frontend_data.builtin_names.directive_section, ".drectve" }, /* original 0xcc470 */
    { &frontend_data.builtin_names.local_static_guard, ".local static guard." }, /* original 0xcc480 */
    { &frontend_data.builtin_names.setjmp_name, "_setjmp" }, /* original 0xcc488 */
    { &frontend_data.builtin_names.catch_name, "Catch" }, /* original 0xcc48c */
    { &frontend_data.builtin_names.local_size, "__LOCAL_SIZE" }, /* original 0xcc474 */
    { &frontend_data.builtin_names.rtti_dynamic_cast, "__RTDynamicCast" }, /* original 0xccad8 */
    { &frontend_data.builtin_names.rtti_cast_to_void, "__RTCastToVoid" }, /* original 0xccadc */
    { &frontend_data.builtin_names.rtti_typeid, "__RTtypeid" }, /* original 0xccae0 */
    { &frontend_data.builtin_names.rtti_typeid_return_type, "__RTtypeidReturnType" }, /* original 0xccae4 */
    { &frontend_data.builtin_names.type_info_vtable, "??_7type_info@@6B@" }, /* original 0xccae8 */
    { &frontend_data.builtin_names.line_variable, "__LINE__Var" }, /* original 0xccb08 */
    { &frontend_data.builtin_names.uuidof_return_type, "__COMuuidofReturnType" }, /* original 0xccb18 */
    { &frontend_data.builtin_names.guid_tag, "__s_GUID" }, /* original 0xccb1c */
    { NULL, NULL }
};
extern "C" void __cdecl initialize_builtin_symbols(void)
{
    const BuiltinNameDescription *entry;
    for (entry = builtin_names; entry->destination; ++entry)
        *entry->destination = find_or_create_identifier(entry->text);
}

extern "C" CompilerType *__cdecl make_function_type(unsigned attributes, CompilerType *base)
{
    CompilerDeclarator declaration;
    CompilerDeclaratorQualifiers qualifiers;
    CompilerParameterDescription *shape;
    CompilerArena *arena = &frontend_bss.arenas[5];
    BYTE *memory;
    declaration.Initialize(base, &frontend_bss.declaration_context);
    memory = arena->cursor;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (memory) {
        shape = (CompilerParameterDescription *)memory;
        shape->kind = 5;
        shape->scope_open = 0;
        shape->parameters = NULL;
    } else {
        shape = NULL;
    }
    declaration.AddFunction(shape);
    qualifiers.flags = (WORD)attributes;
    qualifiers.value = 0;
    declaration.ApplyQualifiers(&qualifiers);
    declaration.Resolve();
    return declaration.type;
}
extern "C" CompilerType *__cdecl make_tagged_type(unsigned flags) { __asm { int 3 } return NULL; }
extern "C" void __cdecl initialize_intermediate_output(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_translation_state(void) { __asm { int 3 } }
extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions) { __asm { int 3 } }
extern "C" void __cdecl begin_line_output(void) { __asm { int 3 } }
extern "C" void __cdecl begin_source_input(void) { __asm { int 3 } }
CompilerTranslationUnit *RECON_THISCALL CompilerTranslationUnit::Initialize(void)
{
    CompilerPool *pool = &frontend_bss.pools[8]; /* original c0b00 */
    CompilerArena *arena = &frontend_bss.arenas[0];
    CompilerNamespaceSymbol *symbol;
    CompilerNameEntry *name;
    void *memory;
    unsigned bytes;
    depth = 0;
    saved_lookup = NULL; retired_scopes = NULL; deferred_symbols = NULL;
    type_aliases = NULL; lookup_context = 1;
    suppress_lookup_errors = 0; lookup_override = 0;
    deferred_scopes = NULL; pending_symbols = NULL; records = NULL;
    if (pool->remaining < 32) pool->Refill(32);
    memory = pool->cursor; pool->cursor += 32; pool->remaining -= 32;
    root = memory ? new(memory) CompilerScope : NULL;
    bytes = root->bucket_mask * 4 + 4;
    if (pool->remaining < bytes) pool->Refill(bytes);
    root->buckets = (CompilerSymbolLink **)pool->cursor;
    pool->cursor += bytes; pool->remaining -= bytes;
    current = root; scopes[0] = root;
    name = frontend_data.builtin_names.global_namespace;
    symbol = (CompilerNamespaceSymbol *)recycle_symbol(0x17, name, 0);
    if (!symbol) {
        memory = arena->Allocate((frontend_data.symbol_record_bytes[0x17] + 3) & ~3U);
        symbol = memory ? (CompilerNamespaceSymbol *)
            ((CompilerSymbol *)memory)->Initialize(0x17, name, 0) : NULL;
    }
    global_namespace = symbol;
    memory = arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    symbol->data = (CompilerNamespaceData *)memory;
    symbol->data->scope = root;
    symbol->definition_flags |= 4;
    return this;
}

extern "C" CompilerSymbol *__cdecl recycle_symbol(unsigned kind, CompilerNameEntry *name, unsigned context)
{
    CompilerSymbolLink *link;
    CompilerNamedSymbol *symbol;
    unsigned index, size;
    if (context) return NULL;
    /* The original indexes by the low byte. The next unmodeled table value
     * is a semantic frontier, not an inferred allocation class. */
    index = kind & 0xff;
    if (index >= 26) { __asm { int 3 } }
    size = frontend_data.symbol_size_class[index];
    link = frontend_data.recycled_symbols[size];
    if (!link) return NULL;
    frontend_data.recycled_symbols[size] = link->next;
    link->name = NULL; link->next = NULL;
    symbol = (CompilerNamedSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
    /* The actual owner has the full per-kind allocation width. Zeroing it
     * does not interpret or claim semantic coverage of its unmodeled tail. */
    memset(symbol, 0, frontend_data.symbol_record_bytes[index]);
    symbol->link.name = name;
    symbol->kind = (BYTE)kind;
    return symbol;
}
CompilerSymbol *RECON_THISCALL CompilerSymbol::Initialize(unsigned kind, CompilerNameEntry *name, unsigned context)
{
    CompilerSymbolLink *binding = &((CompilerNamedSymbol *)this)->link;
    this->kind = (BYTE)kind;
    if (binding) {
        binding->next = NULL;
        binding->name = name;
        if ((BYTE)kind == 4)
            ((CompilerKind4Symbol *)this)->initial_context = 0;
        else if ((BYTE)kind >= 16 && (BYTE)kind <= 17)
            ((CompilerSmallSymbol *)this)->initial_payload = 0;
    }
    return this;
}
int RECON_THISCALL CompilerScope::HasBuckets(void) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerScope::HasSecondary(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerScope::Insert(CompilerSymbol *symbol) { __asm { int 3 } }
CompilerScopeLookup *RECON_THISCALL CompilerScope::Find(CompilerNameEntry *name, unsigned context) { __asm { int 3 } return NULL; }
CompilerSymbol *RECON_THISCALL CompilerScope::FindOrdinary(CompilerNameEntry *name) { __asm { int 3 } return NULL; }
CompilerSymbol *RECON_THISCALL CompilerScope::Lookup(CompilerNameEntry *name, unsigned flags, unsigned *found) { __asm { int 3 } return NULL; }
int RECON_THISCALL CompilerScope::FindNested(CompilerNameEntry *name, CompilerSymbol **found) { __asm { int 3 } return 0; }
CompilerSymbolLink **RECON_THISCALL CompilerScope::FindLink(CompilerSymbolLink *symbol) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerScope::Serialize(void) { __asm { int 3 } }
void RECON_THISCALL CompilerScope::Check(void) { __asm { int 3 } }
void RECON_THISCALL CompilerScope::Link(CompilerSymbol *symbol) { __asm { int 3 } }

extern "C" void __cdecl initialize_first_token(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_language_symbols(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_scope_symbols(void) { __asm { int 3 } }
extern "C" CompilerSymbol *__cdecl lookup_builtin(CompilerNameEntry *name, CompilerDeclarator *lookup) { __asm { int 3 } return NULL; }
extern "C" void __cdecl declare_builtin(CompilerDeclaration *declaration, CompilerSymbol *symbol) { __asm { int 3 } }
CompilerContextBuffer *RECON_THISCALL CompilerContextBuffer::Initialize(unsigned arena_kind) { __asm { int 3 } return this; }
void RECON_THISCALL FrontendContextStack::Push(void *object, unsigned arena, int first, int second, FrontendContextStack *owner) { __asm { int 3 } }
CompilerDeclarator *RECON_THISCALL CompilerDeclarator::Initialize(CompilerType *type, const void *position)
{
    /* Clear only the native typed projection. This makes no claim about the
     * original72-byte object's unmodeled bytes or its28-byte context copy. */
    memset(this, 0, sizeof(*this));
    symbol = make_abstract_declarator();
    context = *(const CompilerDeclarationContext *)position;
    base_type = type;
    return this;
}
void RECON_THISCALL CompilerDeclarator::Resolve(void)
{
    CompilerSymbol *return_type;
    CompilerTypeShape *resolved;
    CompilerParameterDescription *description;
    CompilerMemberSymbol *member;
    unsigned function_attribute;
    int mode;
    if (flags & 0x20) ValidateSpecialFunction();
    if (!qualifiers.flags && !shape) {
        if (base_type->shape)
            base_type->shape = intern_type_shape(base_type->shape);
        base_type = intern_type(base_type);
        flags |= 4;
        type = base_type;
    } else {
        NormalizeBase();
        if (flags & 8) {
            return_type = find_declarator_type(symbol, (context.context_flags >> 2) & 1);
            description = parameters;
            if (return_type && (context.class_flags & 7) != 7) {
                CompilerType *qualified = qualify_type(return_type, 0x100, description->flags);
                shape->SetParameters(description->parameters,
                    link_parameter_symbols(description->context), qualified);
                shape->flags |= 0x2000;
            } else {
                shape->SetParameters(description->parameters,
                    link_parameter_symbols(description->context), NULL);
            }
            if (parameters->flags &&
                ((context.class_flags & 7) == 3 || !return_type)) {
                CompilerNameEntry *name = ((CompilerNamedSymbol *)symbol)->link.name;
                if (name == frontend_data.builtin_names.operator_new ||
                    name == frontend_data.builtin_names.operator_delete ||
                    name == frontend_data.builtin_names.operator_new_array ||
                    name == frontend_data.builtin_names.operator_delete_array)
                    compiler_warning(271, symbol);
                else if ((context.class_flags & 7) == 3)
                    compiler_warning(272, symbol);
                else
                    compiler_warning(270, symbol);
                flags |= 1;
            }
        }
        if ((flags & 0x10) && (last_shape->flags & 0x77) == 4 &&
            !base_type->shape && !base_type->shape &&
            (base_type->flags & 0x400) && (short)base_type->auxiliary >= 0) {
            member = (CompilerMemberSymbol *)frontend_data.frontend_pointer_table->items[
                (short)base_type->auxiliary];
            function_attribute = last_shape->flags & 0x2000;
            if (member->context->flags & 0x80) {
                diagnose_member_declaration(member);
                flags |= 1;
            }
            if ((member->definition_flags & 4) ||
                (function_attribute && (context.class_flags & 7) != 3)) {
                if (!function_attribute || (context.class_flags & 7) == 3) {
                    if (!check_member_declaration(member)) {
                        function_attribute = 0x100;
                        goto set_function_attribute;
                    }
                }
                /* Model only c13d0[-1], not an inferred table capacity. */
                if (frontend_data.declaration_mode_index != -1) { __asm { int 3 } }
                mode = frontend_bss.default_declaration_mode;
                if (mode == 2 &&
                    (!function_attribute || (context.class_flags & 7) == 3))
                    report_language_diagnostic(1, 190, symbol, member);
                function_attribute = 0x200;
            } else {
                if (frontend_data.declaration_mode_index != -1) { __asm { int 3 } }
                mode = frontend_bss.default_declaration_mode;
                function_attribute = mode == 2 ? 0x100 : 0x200;
            }
set_function_attribute:
            last_shape->flags |= function_attribute;
        }
        if (shape_flags & 2) {
            compiler_warning(540);
            flags |= 1;
            shape->flags &= ~0x1000;
            shape->payload.value = 1;
        }
        if (shape_flags & 1) ValidateShapes();
        ValidateContext();
        resolved = shape->InternChain(last_shape ? last_shape->base : NULL);
        type = BuildType(resolved);
    }
    if (!type) {
        flags |= 1;
        type = frontend_data.builtin_types.type_0063;
    }
}
unsigned RECON_THISCALL CompilerLexer::Read(void) { __asm { int 3 } return 0; }
unsigned RECON_THISCALL CompilerLexer::Peek(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Unread(unsigned token) { __asm { int 3 } }
void RECON_THISCALL CompilerLexer::Trace(unsigned token) { __asm { int 3 } }
int RECON_THISCALL CompilerLexer::Valid(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Reset(void) { __asm { int 3 } }
void *RECON_THISCALL CompilerLexer::Destroy(unsigned flags) { __asm { int 3 } return this; }

extern "C" void __cdecl initialize_symbol_groups(void)
{
    frontend_data.symbol_groups = create_hash_table(16, 0, 4, 3, 1, 1);
}
extern "C" CompilerHashTable *__cdecl create_hash_table(unsigned buckets,
    unsigned arena, unsigned hash_kind, unsigned comparison_kind, unsigned flag0, unsigned flag1)
{
    CompilerArena *storage = &frontend_bss.arenas[arena];
    CompilerHashTable *table = (CompilerHashTable *)storage->cursor;
    unsigned bytes;
    BYTE *heads;
    storage->cursor += 24;
    if (storage->cursor > storage->committed_limit) storage->CommitExtent();
    table->comparison_kind = comparison_kind;
    table->hash_kind = hash_kind;
    table->bucket_count = (WORD)buckets;
    table->arena_kind = arena;
    table->mode0 = flag0 & 1;
    table->mode1 = flag1 & 1;
    bytes = (buckets * 4 + 3) & ~3U;
    heads = storage->cursor;
    storage->cursor += bytes;
    if (storage->cursor > storage->committed_limit) storage->CommitExtent();
    table->buckets = (CompilerHashEntry **)heads;
    table->entry_count = 0;
    return table;
}

void RECON_THISCALL CompilerDeclarator::AddFunction(CompilerParameterDescription *description)
{
    int first = shape == NULL;
    unsigned value = 0;
    CompilerTypeShape *function;
    CompilerType *qualified;
    flags |= 0x10;
    description->Normalize();
    function = create_type_shape(4, 0, &value);
    if (first && (qualifiers.flags & 0x70)) {
        function->attributes |= qualifiers.flags & 0x70;
        qualifiers.flags &= ~0x70;
    }
    if (!first) {
        if (!(last_shape->flags & 0x10)) {
            function->SetParameters(description->parameters,
                link_parameter_symbols(description->context), NULL);
            if (description->flags) {
                compiler_warning(270, symbol);
                flags |= 1;
            }
        } else {
            qualified = qualify_type(last_shape->payload.symbol, 0x100, description->flags);
            function->SetParameters(description->parameters,
                link_parameter_symbols(description->context), qualified);
            function->attributes |= 0x2000;
        }
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
        parameters = description;
        flags |= 8;
    }
}
void RECON_THISCALL CompilerParameterDescription::Normalize(void)
{
    if (scope_open) frontend_data.translation_unit->EndParameterScope();
    if (frontend_bss.parser_context_depth >= 1) { __asm { int 3 } }
    context = frontend_bss.parameter_contexts[frontend_bss.parser_context_depth];
    frontend_bss.parameter_contexts[frontend_bss.parser_context_depth] = NULL;
}
CompilerSymbolLink **RECON_THISCALL CompilerTranslationUnit::EndParameterScope(void) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerDeclarator::AppendShape(CompilerTypeShape *next)
{
    shape_flags |= 1;
    if (!shape) return;
    switch (last_shape->flags & 0x77) {
    case 1:
        last_shape->flags = (next->flags & 0xc803) | 3;
        break;
    case 2:
        compiler_warning(92);
        last_shape->payload.value = 0;
        last_shape->flags = 1;
        break;
    case 4: {
        CompilerDeclaratorQualifiers first, second;
        compiler_warning(91);
        first.flags = second.flags = 0;
        first.value = second.value = 0;
        AddPointer(&first, &second);
        last_shape->flags = 3;
        break;
    }
    case 0x10:
        last_shape->flags = (next->flags & 0xc820) | 0x20;
        if (!(next->flags & 0x2000)) {
            next->SetType(qualify_type(last_shape->payload.symbol, 0x100, 0));
            next->flags |= 0x2000;
        }
        break;
    case 0x40:
        last_shape->flags = (next->flags & 0xc844) | 0x44;
        break;
    }
    if (last_shape->attributes & 3) {
        compiler_warning(212, symbol);
        flags |= 1;
        last_shape->attributes &= ~3;
    }
}
void RECON_THISCALL CompilerTypeShape::SetParameters(CompilerParameterSet *parameters,
    CompilerParameterLink *symbols, CompilerType *type)
{
    CompilerFunctionType *function = create_function_parameters(parameters, &flags, symbols, type);
    flags |= 0x400;
    payload.function = function;
}
extern "C" CompilerTypeShape *__cdecl create_type_shape(unsigned flags, unsigned attributes, const unsigned *value)
{
    CompilerArena *arena = &frontend_bss.arenas[5];
    CompilerTypeShape *shape = (CompilerTypeShape *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    shape->flags = (WORD)flags;
    shape->attributes = (WORD)attributes;
    shape->payload.value = *value;
    return shape;
}
extern "C" CompilerType *__cdecl qualify_type(CompilerSymbol *type, unsigned flags, WORD attributes) { __asm { int 3 } return NULL; }
extern "C" CompilerParameterLink *__cdecl link_parameter_symbols(CompilerParameterContext *context)
{
    CompilerParameterEntry *entry;
    CompilerParameterLink *first, *last, *next;
    if (!context) return NULL;
    entry = context->head;
    first = entry->parameter;
    entry = entry->next;
    last = first;
    while (entry) {
        next = entry->parameter;
        last->next = next;
        next->previous = last;
        entry = entry->next;
        last = last->next;
    }
    first->previous = last;
    return first;
}
void RECON_THISCALL CompilerDeclarator::ApplyQualifiers(CompilerDeclaratorQualifiers *value)
{
    if (value->flags & 1) {
        compiler_warning(493);
        flags |= 1;
    }
    qualifiers = *value;
}

extern "C" CompilerSymbol *__cdecl make_abstract_declarator(void)
{
    CompilerAbstractSymbol *symbol = NULL;
    CompilerNameEntry *name = frontend_data.builtin_names.abstract_declarator;
    CompilerArena *arena;
    CompilerSymbolLink *link;
    unsigned arena_kind, size_class;
    BYTE *memory;
    if ((frontend_data.compiler_mode_flags & 0x10000000) ||
        (frontend_data.compiler_flags & 0x48))
        arena_kind = 0;
    else if (frontend_bss.declaration_depth > 0)
        arena_kind = 3 + (frontend_data.declaration_context_index < 0);
    else
        arena_kind = 5;
    if (!arena_kind) {
        size_class = frontend_data.symbol_size_class[13];
        link = frontend_data.recycled_symbols[size_class];
        if (link) {
            frontend_data.recycled_symbols[size_class] = link->next;
            symbol = (CompilerAbstractSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
            link->name = NULL; link->next = NULL;
            memset(symbol, 0, frontend_data.symbol_record_bytes[13]);
            symbol->link.name = name;
            symbol->kind = 13;
        }
    }
    arena = &frontend_bss.arenas[arena_kind];
    if (!symbol) {
        memory = arena->cursor;
        arena->cursor += (frontend_data.symbol_record_bytes[13] + 3) & ~3U;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        symbol = memory ? (CompilerAbstractSymbol *)
            ((CompilerSymbol *)memory)->Initialize(13, name, arena_kind) : NULL;
    }
    memory = arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    symbol->type = (CompilerAbstractType *)memory;
    return symbol;
}

void RECON_THISCALL CompilerDeclarator::AddPointer(CompilerDeclaratorQualifiers *first, CompilerDeclaratorQualifiers *second) { __asm { int 3 } }
void RECON_THISCALL CompilerTypeShape::SetType(CompilerType *type) { __asm { int 3 } }

void RECON_THISCALL CompilerDeclarator::ValidateSpecialFunction(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarator::NormalizeBase(void)
{
    CompilerTypeShape *copy;
    unsigned kind;
    if (base_type->shape) {
        copy = copy_type_shape(base_type->shape);
        switch (copy->flags & 0x77) {
        case 1: case 3: case 0x10: case 0x20:
            break;
        case 2:
            AddArrayShape(copy);
            break;
        case 4:
            AppendShape(copy);
            break;
        case 0x40: case 0x44:
            AddMemberPointerShape(copy);
            break;
        default:
            return;
        }
        if (last_shape) last_shape->base = copy;
        else shape = copy;
        last_shape = copy;
        return;
    }
    if (!shape) return;
    kind = last_shape->flags & 0x77;
    if (!base_type->shape && (base_type->flags & 0x2fdf) == 0x80 &&
        (kind == 2 || kind == 0x10 || kind == 0x40)) {
        compiler_warning(182, symbol);
        flags |= 1;
        base_type = frontend_data.builtin_types.type_0063;
    }
    if ((last_shape->flags & 0x77) == 2 && !base_type->shape &&
        (base_type->flags & 0x400) && (short)base_type->auxiliary >= 0) {
        CompilerMemberSymbol *member = (CompilerMemberSymbol *)frontend_data.frontend_pointer_table->items[
            (short)base_type->auxiliary];
        if (member->declaration_flags & 1) {
            compiler_warning(233, symbol);
            flags |= 1;
        }
    }
}
void RECON_THISCALL CompilerDeclarator::ValidateShapes(void)
{
    CompilerTypeShape *end = last_shape->base;
    CompilerTypeShape *previous = NULL, *entry = shape;
    WORD attributes;
    unsigned kind, warning;
    if (flags & 8) flags |= 2;
    while (entry != end) {
        attributes = entry->attributes & 0x70;
        if (attributes) {
            entry->attributes &= ~0x70;
            warning = 0;
            switch (entry->flags & 0x77) {
            case 1:
                warning = 165;
                break;
            case 3: case 0x20: case 0x44:
                if (entry->base != end)
                    attributes = PropagateQualifiers(entry->base, attributes);
                break;
            case 4:
                attributes = PropagateQualifiers(entry, attributes);
                if (entry != shape) previous->attributes |= attributes;
                break;
            case 0x10:
                warning = 637;
                break;
            case 0x40:
                warning = 532;
                break;
            default:
                goto next;
            }
            if (warning) {
                compiler_warning(warning, 120);
                flags |= 1;
                goto next;
            }
            entry->attributes |= attributes;
        } else if ((entry->flags & 0x77) == 4) {
            if (entry == shape) {
                attributes = qualifiers.flags & 0x70;
                if (attributes) qualifiers.flags &= ~0x70;
                else {
                    flags &= ~2;
                    attributes = DefaultCallingConvention(entry);
                }
            } else {
                attributes = previous->attributes & 0x70;
                if (!attributes) attributes = DefaultCallingConvention(entry);
                previous->attributes |= attributes;
            }
            entry->attributes |= attributes;
        }
next:
        previous = entry;
        entry = entry->base;
    }
}
void RECON_THISCALL CompilerDeclarator::ValidateContext(void) { __asm { int 3 } }
CompilerType *RECON_THISCALL CompilerDeclarator::BuildType(CompilerTypeShape *shape) { __asm { int 3 } return NULL; }
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::InternChain(CompilerTypeShape *stop) { __asm { int 3 } return NULL; }
extern "C" CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *shape) { __asm { int 3 } return NULL; }
extern "C" CompilerSymbol *__cdecl find_declarator_type(CompilerSymbol *symbol, unsigned context)
{
    CompilerSymbolLink *next = ((CompilerNamedSymbol *)symbol)->link.next;
    CompilerNamedSymbol *linked = next ? (CompilerNamedSymbol *)
        ((BYTE *)next - offsetof(CompilerNamedSymbol, link)) : NULL;
    CompilerSymbolBinding *binding;
    CompilerSymbol *result;
    if (linked) {
        binding = ((CompilerBoundSymbol *)linked)->binding;
        if (binding->owner->scope) {
            binding = ((CompilerBoundSymbol *)linked)->binding;
            if (binding) symbol = linked;
        }
    }
    if (symbol->kind != 0x12) {
        if (frontend_data.declaration_context_index < 0 ||
            frontend_bss.declaration_depth || context)
            return NULL;
        return current_declarator_context();
    }
    binding = ((CompilerBoundSymbol *)symbol)->binding;
    result = binding ? binding->owner->scope->symbol : current_declarator_context();
    if (!result || result->kind == 0x17) return NULL;
    if (result->kind != 4)
        result = (CompilerSymbol *)frontend_data.frontend_pointer_table->items[
            ((CompilerIndexedTypeSymbol *)result)->type_index];
    return result;
}
extern "C" int __cdecl check_member_declaration(CompilerSymbol *symbol) { __asm { int 3 } return 0; }
extern "C" void __cdecl diagnose_member_declaration(CompilerSymbol *symbol) { __asm { int 3 } }

extern "C" CompilerTypeShape *__cdecl copy_type_shape(CompilerTypeShape *shape) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerDeclarator::AddArrayShape(CompilerTypeShape *shape) { __asm { int 3 } }
void RECON_THISCALL CompilerDeclarator::AddMemberPointerShape(CompilerTypeShape *shape) { __asm { int 3 } }

extern "C" CompilerSymbol *__cdecl current_declarator_context(void) { __asm { int 3 } return NULL; }

extern "C" CompilerFunctionType *__cdecl create_function_parameters(CompilerParameterSet *parameters,
    WORD *flags, CompilerParameterLink *symbols, CompilerType *type)
{
    CompilerParameterTypes *types;
    CompilerArena *arena;
    CompilerFunctionType *function;
    unsigned count = 0;
    int empty = parameters == NULL;
    if (parameters) {
        CompilerSymbolLink *entry = parameters->head;
        CompilerTypedSymbol *last;
        int void_parameter = 0;
        while (entry) {
            last = (CompilerTypedSymbol *)((BYTE *)entry - offsetof(CompilerNamedSymbol, link));
            if (last->type == frontend_data.builtin_types.type_0080)
                void_parameter = 1;
            ++count;
            if (!last->type) {
                *flags |= 0x8000;
                break;
            }
            entry = entry->next;
        }
        if (void_parameter) {
            if (count != 1) compiler_warning(860);
            last->definition_flags |= 8;
            empty = 1;
        }
    }
    if (empty) {
        *flags |= 0x4000;
        types = empty_parameter_types();
    } else {
        types = collect_parameter_types(count, parameters);
    }
    arena = &frontend_bss.arenas[5];
    function = (CompilerFunctionType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    function->parameters = types;
    if (symbols) {
        BYTE total = 0;
        CompilerParameterLink *entry = symbols;
        function->symbols = symbols;
        do {
            ++total;
            entry = entry->next;
        } while (entry);
        function->symbol_count = total;
    }
    function->type = type;
    return intern_function_type(function);
}

extern "C" CompilerParameterTypes *__cdecl empty_parameter_types(void)
{
    if (!frontend_data.empty_parameters) {
        CompilerArena *arena = &frontend_bss.arenas[0];
        CompilerParameterTypes *parameters = (CompilerParameterTypes *)arena->cursor;
        arena->cursor += 12; /* original allocation; +4 has no native field */
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        frontend_data.empty_parameters = parameters;
        parameters->count = 1;
        parameters->types[0] = frontend_data.builtin_types.type_0080;
    }
    return frontend_data.empty_parameters;
}
extern "C" CompilerParameterTypes *__cdecl collect_parameter_types(unsigned count, CompilerParameterSet *parameters) { __asm { int 3 } return NULL; }
extern "C" CompilerFunctionType *__cdecl intern_function_type(CompilerFunctionType *function)
{
    unsigned hash = (((WORD)(unsigned)function->type >> 2) +
        ((WORD)(unsigned)function->parameters >> 2)) & 0x3ff;
    CompilerCompositeType **bucket = &frontend_data.type_tables.composite->buckets[hash];
    CompilerCompositeType *previous = NULL, *entry = *bucket, *created;
    CompilerPool *pool = &frontend_bss.pools[3]; /* actual bf210 */
    int count = (int)function->parameters->count;
    while (entry) {
        int other_count = (int)entry->function.parameters->count;
        if (other_count > count) break;
        if (other_count == count) {
#if defined(RECON_NATIVE_TEST)
            /* The original compares all16 bytes. The three bytes adjacent
             * to symbol_count are unresolved: stop before interpreting them,
             * rather than replacing the comparison with typed equality. */
            __asm { int 3 }
#endif
            if (!memcmp(&entry->function, function, 16)) return &entry->function;
        }
        previous = entry;
        entry = entry->next;
    }
    if (pool->remaining < 20) pool->Refill(20);
    created = (CompilerCompositeType *)pool->cursor;
    pool->cursor += 20;
    pool->remaining -= 20;
    created->function = *function;
    created->next = entry;
    if (previous) previous->next = created;
    else *bucket = created;
    return &created->function;
}

WORD RECON_THISCALL CompilerDeclarator::DefaultCallingConvention(CompilerTypeShape *shape) { __asm { int 3 } return 0; }
WORD RECON_THISCALL CompilerDeclarator::PropagateQualifiers(CompilerTypeShape *shape, WORD attributes) { __asm { int 3 } return 0; }
