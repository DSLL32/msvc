#include <io.h>
#include <new.h>
#include <stddef.h>
#include <string.h>
#include "frontend-init.h"
#include "pointer-array.h"
#include "packing.h"
#pragma intrinsic(memset, memcmp, memcpy)
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
int __cdecl format_compiler_text(char *, const char *, unsigned, ...);
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
WORD __cdecl normalize_shape_attributes(unsigned, WORD);
CompilerTypeShape *__cdecl create_declarator_shape(unsigned, WORD, void *);
void __cdecl initialize_intermediate_output(void);
void __cdecl initialize_translation_state(void);
void *__cdecl allocate_symbol_storage(unsigned, unsigned, unsigned);
CompilerVector *__cdecl new_option_vector(unsigned, int);
void __cdecl define_command_line_macros(CompilerVector *);
void __cdecl begin_line_output(void);
void __cdecl begin_source_input(void);
void __cdecl initialize_first_token(void);
void __cdecl initialize_symbol_groups(void);
void __cdecl initialize_language_symbols(void);
void __cdecl initialize_purecall_symbol(void);
CompilerSymbol *__cdecl lookup_builtin(CompilerNameEntry *);
CompilerSymbol *__cdecl declare_builtin(CompilerDeclaration *, CompilerSymbol *, CompilerDeclarator *);
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
        format_compiler_text(processor_switch, "-G%d", 20, frontend_data.options.processor);
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
            initialize_purecall_symbol();
            if (frontend_data.options.language_extensions && !frontend_data.options.no_bool) {
                /* Original3a3d5 clears40 bytes. The grounded28-byte context
                 * is inside that span; the declaration tail stays unmodeled. */
                memset(&declaration.context, 0, sizeof(declaration.context));
                declaration.context.class_flags = 7;
                declaration.type = types->type_0060;
                lookup.Initialize(types->type_0060, &frontend_bss.declaration_context);
                lookup.Resolve();
                declare_builtin(&declaration,
                    lookup_builtin(frontend_data.bool_identifier), &lookup);
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
extern "C" int __cdecl format_compiler_text(char *, const char *, unsigned, ...) { __asm { int 3 } return 0; }
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
extern "C" CompilerType *__cdecl make_tagged_type(unsigned flags)
{
    CompilerArena *arena;
    CompilerType *description;
    if (flags & 0x80) arena = &frontend_bss.arenas[5];
    else {
        if (frontend_data.tagged_type_arena_index >= 9) { __asm { int 3 } }
        arena = &frontend_bss.arenas[frontend_data.tagged_type_arena_index];
    }
    description = (CompilerType *)arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    description->flags = (WORD)flags;
    if (flags & 0x80) {
        description->attributes = 0;
        description->auxiliary = 0;
        description->shape = intern_type_shape(create_declarator_shape(1,
            normalize_shape_attributes(1, 0), NULL));
    }
    return intern_type(description);
}
extern "C" void __cdecl initialize_intermediate_output(void) {}
extern "C" void __cdecl initialize_translation_state(void)
{
    CompilerTranslationSymbol *symbol = NULL;
    unsigned size_class = frontend_data.symbol_size_class[9];
    CompilerSymbolLink *link;
    CompilerNameEntry *name;
    void *memory;
    if (size_class >= 11) { __asm { int 3 } }
    link = frontend_data.recycled_symbols[size_class];
    if (link) {
        frontend_data.recycled_symbols[size_class] = link->next;
        symbol = (CompilerTranslationSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
        link->name = NULL;
        link->next = NULL;
        memset(symbol, 0, frontend_data.symbol_record_bytes[9]);
        symbol->link.name = NULL;
        symbol->kind = 9;
    }
    if (!symbol) {
        memory = allocate_symbol_storage(48, 9, 0);
        symbol = memory ? (CompilerTranslationSymbol *)
            ((CompilerSymbol *)memory)->Initialize(9, NULL, 0) : NULL;
    }
    frontend_data.unnamed_translation_symbol = symbol;
    symbol->state = NULL;
    symbol->type = frontend_data.builtin_types.type_0000;
    symbol->attributes |= 0x20;
    symbol->definition_flags |= 4;
    symbol->encoded_index = (symbol->encoded_index & 0xff000001) | 1;
    symbol->emission_state = (symbol->emission_state & 7) | 8;

    symbol = NULL;
    size_class = frontend_data.symbol_size_class[9];
    name = frontend_data.builtin_names.local_size;
    if (size_class >= 11) { __asm { int 3 } }
    link = frontend_data.recycled_symbols[size_class];
    if (link) {
        frontend_data.recycled_symbols[size_class] = link->next;
        symbol = (CompilerTranslationSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
        link->name = NULL;
        link->next = NULL;
        memset(symbol, 0, frontend_data.symbol_record_bytes[9]);
        symbol->link.name = name;
        symbol->kind = 9;
    }
    if (!symbol) {
        memory = frontend_bss.arenas[0].Allocate((frontend_data.symbol_record_bytes[9] + 3) & ~3U);
        symbol = memory ? (CompilerTranslationSymbol *)
            ((CompilerSymbol *)memory)->Initialize(9, name, 0) : NULL;
    }
    frontend_data.local_size_symbol = symbol;
    symbol->type = frontend_data.builtin_types.type_0000;
    symbol->encoded_index = (symbol->encoded_index & 0xff00000a) | 0xa;
    symbol->emission_state = (symbol->emission_state & 7) | 0x50;
    symbol->attributes |= 0x20;
    symbol->definition_flags |= 4;
    frontend_data.translation_symbols = new_option_vector(1, 0);
}
extern "C" void __cdecl begin_line_output(void) { __asm { int 3 } }
/* Debug/backend initialization is defined in debug-init.cpp. */
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
    deferred_scopes = NULL; pending_symbols = NULL; source_files = NULL;
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

extern "C" CompilerSymbol *__cdecl allocate_parser_symbol(unsigned kind, CompilerNameEntry *name, unsigned arena_index)
{
    unsigned index = kind & 0xff;
    CompilerNamedSymbol *symbol = NULL;
    CompilerSymbolLink *link;
    CompilerArena *arena;
    unsigned size_class;
    if (index >= 26 || arena_index >= 9) { __asm { int 3 } }
    if (!arena_index) {
        size_class = frontend_data.symbol_size_class[index];
        if (size_class >= 11) { __asm { int 3 } }
        link = frontend_data.recycled_symbols[size_class];
        if (link) {
            frontend_data.recycled_symbols[size_class] = link->next;
            link->name = NULL;
            link->next = NULL;
            symbol = (CompilerNamedSymbol *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
            memset(symbol, 0, frontend_data.symbol_record_bytes[index]);
            link->name = name;
            symbol->kind = (BYTE)kind;
        }
    }
    if (symbol) return symbol;
    arena = &frontend_bss.arenas[arena_index];
    symbol = (CompilerNamedSymbol *)arena->cursor;
    arena->cursor += (frontend_data.symbol_record_bytes[index] + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!symbol) return NULL;
    symbol->kind = (BYTE)kind;
    link = &symbol->link;
    if (link) {
        link->name = name;
        link->next = NULL;
        if ((BYTE)kind == 4) link->SetClassContext(NULL);
        else if ((BYTE)kind >= 16 && (BYTE)kind <= 17)
            ((CompilerSmallSymbol *)symbol)->initial_payload = 0;
    }
    return symbol;
}

void RECON_THISCALL CompilerSymbol::MarkParameter(void)
{
    definition_flags |= 8;
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
int RECON_THISCALL CompilerScope::HasBuckets(void)
{
    return buckets || secondary ? 1 : 0;
}
int RECON_THISCALL CompilerScope::HasSecondary(void) { __asm { int 3 } return 0; }
CompilerSymbol *RECON_THISCALL CompilerParameterSet::Find(CompilerNameEntry *name)
{
    CompilerSymbolLink *link = head;
    while (link) {
        if (link->name == name) {
            CompilerSymbol *symbol = (CompilerSymbol *)((BYTE *)link - 8);
            if (symbol->kind != 24 || frontend_data.translation_unit->lookup_override)
                return symbol;
            CompilerSymbol *target = ((CompilerAliasSymbol *)symbol)->target->symbol;
            unsigned kind = target->kind;
            if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25)
                return symbol;
            if (kind == 24) {
                target = ((CompilerAliasSymbol *)target)->target->symbol;
                kind = target->kind;
                if (kind == 16 || kind == 7 || kind == 6 || kind == 17 || kind == 8 || kind == 25)
                    return symbol;
            }
            while (symbol->kind == 24)
                symbol = ((CompilerAliasSymbol *)symbol)->target->symbol;
            return symbol;
        }
        link = link->next;
    }
    return NULL;
}

CompilerScopeLookup *RECON_THISCALL CompilerAuxiliaryScope::Find(CompilerNameEntry *name, unsigned)
{
    if (!parameters) return NULL;
    CompilerSymbol *symbol = parameters->Find(name);
    if (!symbol) return NULL;
    CompilerArena *arena = &frontend_bss.arenas[5];
    CompilerScopeLookup *lookup = (CompilerScopeLookup *)arena->cursor;
    arena->cursor += 8;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!lookup) return NULL;
    CompilerScopeMatch *match = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (match) {
        match->symbol = symbol;
        match->scope = this;
        match->next = NULL;
    }
    lookup->head = lookup->tail = match;
    return lookup;
}

CompilerScopeMatch *RECON_THISCALL CompilerScope::FindOrdinary(CompilerNameEntry *name)
{
    CompilerSymbolLink *link;
    CompilerNamedSymbol *symbol = NULL;
    CompilerScopeMatch *match;
    CompilerArena *arena = &frontend_bss.arenas[5];
    if (buckets) {
        link = buckets[(bucket_mask & name->flags) & 0xffff];
        while (link) {
            if (link->name == name) {
                CompilerNamedSymbol *candidate = (CompilerNamedSymbol *)
                    ((BYTE *)link - offsetof(CompilerNamedSymbol, link));
                if (candidate->kind == 4 || candidate->kind == 0x17) {
                    symbol = candidate;
                    break;
                }
            }
            link = link->next;
        }
    }
    if (!symbol) {
        if (level == 2) return next->FindOrdinary(name);
        return NULL;
    }
    match = (CompilerScopeMatch *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (!match) return NULL;
    match->symbol = symbol;
    match->scope = this;
    match->next = NULL;
    return match;
}

int RECON_THISCALL CompilerScope::FindNested(CompilerNameEntry *name, CompilerSymbol **found) { __asm { int 3 } return 0; }
CompilerSymbolLink **RECON_THISCALL CompilerScope::FindLink(CompilerSymbolLink *symbol) { __asm { int 3 } return NULL; }
void RECON_THISCALL CompilerScope::Serialize(void) { __asm { int 3 } }
void RECON_THISCALL CompilerScope::Link(CompilerSymbol *symbol)
{
    CompilerSymbolLink *link = &((CompilerNamedSymbol *)symbol)->link;
    unsigned index = (bucket_mask & link->name->flags) & 0xffff;
    link->next = buckets[index];
    buckets[index] = link;
}

extern "C" CompilerNameEntry *__cdecl operator_identifier(unsigned);
extern "C" void *__cdecl make_integer_constant(int);
extern "C" void __cdecl initialize_first_token(void)
{
    CompilerPredefinedOperator *op;
    unsigned i;
    frontend_data.arrow_identifier = operator_identifier(83);
    frontend_data.call_identifier = operator_identifier(118);
    for (op = frontend_data.predefined_operators; op->token; ++op) {
        op->identifier = operator_identifier(op->token);
        for (i = 0; i < 2; ++i)
            if (op->operands[i].code)
                op->operands[i].type = make_tagged_type(op->operands[i].code);
    }
    frontend_data.source_mode_flags |= 0x10;
    frontend_data.tagged_type_arena_index = 0;
    frontend_data.constant_arena_index = 0;
    frontend_data.zero_constant = (CompilerExpression *)make_integer_constant(0);
    frontend_data.source_mode_flags &= ~0x10U;
    unsigned flags = frontend_data.compiler_mode_flags;
    unsigned mode = frontend_data.source_mode_flags;
    unsigned compiler_flags = frontend_data.compiler_flags;
    if ((flags & 0x32000000) ||
        ((flags & 0x40000000) && !(mode & 8) && !(compiler_flags & 0x200)) ||
        (mode & 0x10) || ((flags & 0x4000000) && !(compiler_flags & 0x8000))) {
        frontend_data.tagged_type_arena_index = 0;
        frontend_data.constant_arena_index = 0;
    } else {
        frontend_data.tagged_type_arena_index = 5;
        frontend_data.constant_arena_index = 5;
        if (mode & 1) {
            frontend_data.tagged_type_arena_index = 4;
            frontend_data.constant_arena_index = 4;
        }
    }
}
extern "C" void __cdecl declare_vector_helper(CompilerNameEntry *, unsigned, int);
extern "C" void __cdecl initialize_language_symbols(void)
{
    declare_vector_helper(frontend_data.builtin_names.vector_constructor, 11, 1);
    declare_vector_helper(frontend_data.builtin_names.vector_vbase_constructor, 12, 1);
    declare_vector_helper(frontend_data.builtin_names.vector_destructor, 13, 1);
    declare_vector_helper(frontend_data.builtin_names.eh_vector_constructor, 14, 0);
    declare_vector_helper(frontend_data.builtin_names.eh_vector_vbase_constructor, 15, 0);
    declare_vector_helper(frontend_data.builtin_names.eh_vector_destructor, 16, 0);
}
extern "C" void __cdecl declare_runtime_helper(CompilerNameEntry *, unsigned, unsigned, unsigned, unsigned, int);
extern "C" void __cdecl declare_vector_helper(CompilerNameEntry *name, unsigned operation, int enabled)
{
    declare_runtime_helper(name, 0, 0x70, 0x60, operation, enabled);
}
extern "C" CompilerType *__cdecl runtime_helper_return_type(unsigned);
extern "C" CompilerType *__cdecl runtime_helper_parameter_type(unsigned, unsigned);
extern "C" void __cdecl declare_runtime_helper(CompilerNameEntry *name, unsigned return_mode,
    unsigned parameter_mode, unsigned unused_qualifiers, unsigned operation, int enabled)
{
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)lookup_builtin(name);
    CompilerArena *arena = &frontend_bss.arenas[5]; /* own c65a0 */
    CompilerParameterDescription *parameters;
    CompilerDeclarator lookup;
    CompilerDeclaratorQualifiers qualifiers;
    CompilerDeclaration declaration;
    symbol->scope_flags |= 1;
    parameters = (CompilerParameterDescription *)arena->cursor;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (parameters) {
        parameters->kind = 4;
        parameters->scope_open = 0;
        parameters->parameters = NULL;
    }
    parameters->Append(0, runtime_helper_return_type(return_mode));
    parameters->Append(0, frontend_data.builtin_types.type_4063_second); /* actual cc510 */
    parameters->Append(0, frontend_data.builtin_types.type_0063); /* actual cc4c0 */
    parameters->Append(0, runtime_helper_parameter_type(return_mode, parameter_mode));
    if (operation == 14 || operation == 15)
        parameters->Append(0, runtime_helper_parameter_type(return_mode, parameter_mode));
    lookup.InitializeSymbol(symbol);
    qualifiers.flags = 0x60;
    qualifiers.value = 0;
    lookup.ApplyQualifiers(&qualifiers);
    lookup.AddFunction(parameters);
    /* Original490df clears exactly the28-byte context, then stores type. */
    memset(&declaration.context, 0, sizeof(declaration.context));
    declaration.context.class_flags = 1;
    declaration.context.context_flags = 0;
    if (enabled) declaration.context.class_flags |= 0x40;
    declaration.type = frontend_data.builtin_types.type_0080; /* actual cc518 */
    lookup.ApplyDeclaration(&declaration);
    lookup.Resolve();
    symbol->builtin_operation = (BYTE)operation;
    if (operation && (operation <= 13 || operation > 16)) symbol->builtin_flags |= 8;
    else symbol->builtin_flags &= ~8;
    declare_builtin(&declaration, symbol, &lookup);
}
extern "C" CompilerType *__cdecl runtime_helper_return_type(unsigned attributes)
{
    CompilerDeclarator declaration;
    CompilerDeclaratorQualifiers first, second;
    declaration.Initialize(frontend_data.builtin_types.type_0080, &frontend_bss.declaration_context); /* cc518 */
    first.flags = (WORD)attributes;
    first.value = 0;
    second.flags = 0;
    second.value = 0;
    declaration.AddPointer(&first, &second);
    declaration.Resolve();
    return declaration.type;
}
extern "C" CompilerType *__cdecl runtime_helper_parameter_type(unsigned return_mode, unsigned parameter_mode)
{
    CompilerArena *arena = &frontend_bss.arenas[5]; /* actual c65a0 */
    CompilerParameterDescription *parameters = (CompilerParameterDescription *)arena->cursor;
    CompilerDeclarator declaration;
    CompilerDeclaratorQualifiers first, second;
    arena->cursor += 28;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (parameters) {
        parameters->kind = 4;
        parameters->scope_open = 0;
        parameters->parameters = NULL;
    }
    parameters->Append(NULL, runtime_helper_return_type(return_mode));
    declaration.Initialize(frontend_data.builtin_types.type_0080, &frontend_bss.declaration_context);
    first.flags = (WORD)parameter_mode;
    first.value = 0;
    second.flags = 0;
    second.value = 0;
    declaration.AddPointer(&first, &second);
    declaration.AddFunction(parameters);
    declaration.Resolve();
    return declaration.type;
}
extern "C" CompilerSymbol *__cdecl make_parameter_symbol(CompilerNameEntry *, CompilerType *);
void RECON_THISCALL CompilerParameterDescription::Append(CompilerNameEntry *name, CompilerType *type)
{
    CompilerSymbol *symbol = make_parameter_symbol(name, type);
    AppendSymbol(symbol);
}
extern "C" void __cdecl finalize_symbol_declaration(CompilerDeclarator *, CompilerSymbol *);
extern "C" CompilerSymbol *__cdecl make_parameter_symbol(CompilerNameEntry *name, CompilerType *type)
{
    CompilerParameterSymbol *symbol = (CompilerParameterSymbol *)recycle_symbol(11, name, 4);
    if (!symbol) {
        BYTE *memory = frontend_bss.arenas[4].Allocate(
            (frontend_data.symbol_record_bytes[11] + 3) & ~3U);
        symbol = memory ? (CompilerParameterSymbol *)
            ((CompilerSymbol *)memory)->Initialize(11, name, 4) : NULL;
    }
    BYTE *state = (BYTE *)&symbol->declaration_state;
    BYTE classification = (*state & ~7) ^ 5;
    *state = classification;
    symbol->declaration_state = 0;
    *state = (*state & ~7) | (classification & 7);
    symbol->type = type;
    symbol->scope_flags |= 1;
    if (frontend_data.next_symbol_record_id >= 0x7fffffff)
        report_compiler_error(55);
    unsigned index = frontend_data.next_symbol_record_id++;
    symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    symbol->scope_flags |= 8;
    finalize_symbol_declaration(NULL, symbol);
    return symbol;
}
extern "C" CompilerSymbol *__cdecl make_parameter_sentinel(void);
void RECON_THISCALL CompilerParameterDescription::AppendSymbol(CompilerSymbol *symbol)
{
    CompilerParameterSet *set;
    CompilerSymbolLink *link;
    CompilerArena *arena;
    CompilerPool *pool;
    BYTE *memory;
    unsigned arena_index;
    if (!parameters) {
        arena_index = (frontend_data.declaration_context_index < 0) + 3;
        if (!arena_index && frontend_data.free_parameter_sets) {
            set = frontend_data.free_parameter_sets;
            frontend_data.free_parameter_sets = set->next_free;
            set->arena_index = 0;
            set->head = set->tail = NULL;
        } else {
            if (arena_index) {
                arena = &frontend_bss.arenas[arena_index];
                memory = arena->cursor;
                arena->cursor += 12;
                if (arena->cursor > arena->committed_limit) arena->CommitExtent();
            } else {
                pool = &frontend_bss.pools[8]; /* actual c0b00 */
                if (pool->remaining < 12) pool->Refill(12);
                memory = pool->cursor;
                pool->cursor += 12;
                pool->remaining -= 12;
            }
            set = (CompilerParameterSet *)memory;
            if (set) {
                set->arena_index = arena_index;
                set->head = set->tail = NULL;
            }
        }
        parameters = set;
    }
    set = parameters;
    link = &((CompilerNamedSymbol *)symbol)->link;
    if (link) {
        link->name = link->name;
        link->next = NULL;
        ((CompilerParameterSymbol *)symbol)->previous_parameter = set->tail;
    }
    if (!set->tail) set->head = set->tail = link;
    else {
        if (!frontend_data.parameter_sentinel)
            frontend_data.parameter_sentinel = make_parameter_sentinel();
        if ((CompilerSymbol *)((BYTE *)set->tail - offsetof(CompilerNamedSymbol, link)) ==
            frontend_data.parameter_sentinel) set->head = set->tail = link;
        else { set->tail->next = link; set->tail = link; }
    }
}
extern "C" CompilerSymbol *__cdecl make_parameter_sentinel(void)
{
    CompilerNameEntry *name = frontend_data.builtin_names.formal_parameter;
    CompilerIrSymbol *symbol = (CompilerIrSymbol *)recycle_symbol(12, name, 0);
    if (!symbol) {
        BYTE *memory = frontend_bss.arenas[0].Allocate(
            (frontend_data.symbol_record_bytes[12] + 3) & ~3U);
        symbol = memory ? (CompilerIrSymbol *)
            ((CompilerSymbol *)memory)->Initialize(12, name, 0) : NULL;
    }
    BYTE *classification = (BYTE *)&symbol->declaration_state;
    BYTE kind = (*classification & ~7) ^ 4;
    *classification = kind;
    symbol->declaration_state = 0;
    *classification = (*classification & ~7) | (kind & 7);
    if (frontend_data.next_symbol_record_id >= 0x7fffffff)
        report_compiler_error(55);
    unsigned index = frontend_data.next_symbol_record_id++;
    symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    if ((symbol->emission_state & 3) < 3)
        symbol->emission_state = (symbol->emission_state & ~3U) | ((symbol->emission_state + 1) & 3);
    symbol->emission_state = (symbol->emission_state & ~4U) | ((frontend_data.reference_context & 1) << 2);
    symbol->type = frontend_data.builtin_types.type_0080;
    return symbol;
}
CompilerDeclarator *RECON_THISCALL CompilerDeclarator::InitializeSymbol(CompilerSymbol *value)
{
    /* The original clears72 bytes. Clear every modeled declarator field
     * and copy the grounded28-byte context; the remaining object bytes
     * still have no native storage or equality claim. */
    memset(this, 0, sizeof(*this));
    if (!value) value = make_abstract_declarator();
    symbol = value;
    context = frontend_bss.declaration_context;
    return this;
}
void RECON_THISCALL CompilerDeclarator::ApplyDeclaration(CompilerDeclaration *declaration)
{
    if (!base_type) base_type = declaration->type;
    else if (!(declaration->context.class_flags & 0x10)) {
        if (flags & 0x20) compiler_warning(549);
        flags |= 1;
    }
    /* Copy the single grounded28-byte context owner. The declaration
     * tail is separate and remains unmodeled. */
    context = declaration->context;
}

extern "C" CompilerSymbol *__cdecl create_runtime_symbol(unsigned);
extern "C" void __cdecl initialize_purecall_symbol(void)
{
    frontend_data.purecall_symbol = create_runtime_symbol(0);
}
extern "C" CompilerSymbol *__cdecl declare_external_symbol(CompilerNameEntry *, unsigned);
/* Encountered descriptor d9180: name at0 and convention WORD at4.
 * Its remaining original row bytes and all later indices are unmodeled. */
struct RuntimeSymbolDescription { const char *name; WORD convention; };
static const RuntimeSymbolDescription runtime_symbols[] = { { "_purecall", 0x10 } };
extern "C" CompilerSymbol *__cdecl create_runtime_symbol(unsigned index)
{
#ifdef RECON_NATIVE_TEST
    if (index != 0) recon_unsupported("runtime symbol: unmodeled descriptor index");
#endif
    const RuntimeSymbolDescription *description = &runtime_symbols[index];
    CompilerRuntimeFunction *symbol = (CompilerRuntimeFunction *)declare_external_symbol(
        find_or_create_identifier(description->name), 0);
    symbol->kind = 7;
    symbol->type = make_function_type(description->convention, frontend_data.builtin_types.type_0063);
    symbol->type_flags = (symbol->type_flags & ~1) | 2;
    symbol->function_class = 1;
    return symbol;
}
extern "C" CompilerSymbol *__cdecl declare_external_symbol(CompilerNameEntry *name, unsigned mode)
{
    BYTE local = (BYTE)mode;
    unsigned arena_index = local ? 4 : 0;
    CompilerIrSymbol *symbol;
#ifdef RECON_NATIVE_TEST
    if (local) recon_unsupported("external declaration: unvisited local publication mode");
#endif
    symbol = (CompilerIrSymbol *)recycle_symbol(10, name, arena_index);
    if (!symbol) {
        BYTE *memory = frontend_bss.arenas[arena_index].Allocate(
            (frontend_data.symbol_record_bytes[10] + 3) & ~3U);
        symbol = memory ? (CompilerIrSymbol *)((CompilerSymbol *)memory)->Initialize(10, name, arena_index) : NULL;
    }
    BYTE *classification = (BYTE *)&symbol->declaration_state;
    BYTE storage = (*classification & ~7) ^ (local ? 4 : 1);
    *classification = storage;
    if (local == 1) local = 0;
    symbol->declaration_state = 0;
    *classification = (*classification & ~7) | (storage & 7);
    if (frontend_data.next_symbol_record_id >= 0x7fffffff) report_compiler_error(55);
    unsigned index = frontend_data.next_symbol_record_id++;
    symbol->encoded_index = (symbol->encoded_index & 0xff000000) | (index & 0xffffff);
    symbol->emission_state &= ~7;
    symbol->type = frontend_data.builtin_types.type_0063;
    frontend_bss.external_declaration_mode = local;
    (local ? frontend_data.translation_unit->current : frontend_data.translation_unit->root)->Insert(symbol);
    return symbol;
}
extern "C" CompilerSymbol *__cdecl lookup_builtin(CompilerNameEntry *name)
{
    unsigned arena_kind = 0, size_class;
    CompilerRuntimeFunction *symbol = NULL;
    CompilerSymbolLink *link;
    CompilerArena *arena;
    BYTE *memory;
    unsigned bytes;
    if (frontend_bss.declaration_scope_mode && frontend_data.declaration_context_index < 0 &&
        !(frontend_data.compiler_mode_flags & 0x10000000)) {
        if (!(frontend_data.compiler_mode_flags & 0x40000000) ||
            (frontend_data.source_mode_flags & 8) || (frontend_data.compiler_flags & 0x200)) {
            if (!(frontend_data.source_mode_flags & 0x50)) arena_kind = 4;
        }
    }
    if (!arena_kind) {
        size_class = frontend_data.symbol_size_class[1];
        link = frontend_data.recycled_symbols[size_class];
        if (link) {
            frontend_data.recycled_symbols[size_class] = link->next;
            link->name = NULL;
            link->next = NULL;
            symbol = (CompilerRuntimeFunction *)((BYTE *)link - offsetof(CompilerNamedSymbol, link));
            /* The original clears its real per-kind owned allocation. Its
             * unmodeled tail remains uninterpreted; this is not a layout claim. */
            memset(symbol, 0, frontend_data.symbol_record_bytes[1]);
            symbol->link.name = name;
            symbol->kind = 1;
        }
    }
    if (!symbol) {
        arena = &frontend_bss.arenas[arena_kind];
        memory = arena->cursor;
        bytes = (frontend_data.symbol_record_bytes[1] + 3) & ~3U;
        arena->cursor += bytes;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        symbol = memory ? (CompilerRuntimeFunction *)
            ((CompilerSymbol *)memory)->Initialize(1, name, arena_kind) : NULL;
    }
    arena = &frontend_bss.arenas[frontend_data.tagged_type_arena_index];
    memory = arena->cursor;
    arena->cursor += 16;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    symbol->payload = (CompilerSymbolPayload *)memory;
    symbol->function_class = 1;
    return symbol;
}
CompilerContextBuffer *RECON_THISCALL CompilerContextBuffer::Initialize(unsigned kind)
{
#ifdef RECON_NATIVE_TEST
    if (kind >= 9) recon_unsupported("context buffer: unsupported arena index");
#endif
    CompilerArena *arena = &frontend_bss.arenas[kind];
    position = 4;
    DWORD *memory = (DWORD *)arena->cursor;
    arena->cursor += 128;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (memory) memory[0] = 0;
    block = memory;
    arena_kind = kind;
    return this;
}
typedef char FrontendContextFrame_layout[(sizeof(FrontendContextFrame) == 24 &&
    offsetof(FrontendContextFrame, callback) == 20) ? 1 : -1];
void RECON_THISCALL FrontendContextStack::Push(void *object, unsigned arena_kind,
    int first, FrontendContextPopCallback callback, FrontendContextStack *owner)
{
    /* The reached caller supplies the lexer whose header owns these fields. */
    CompilerLexer *lexer = (CompilerLexer *)object;
    if (arena_kind == 9) arena_kind = lexer->arena_kind;
#ifdef RECON_NATIVE_TEST
    if (arena_kind >= 9) recon_unsupported("context stack: unsupported arena index");
#endif
    CompilerArena *arena = &frontend_bss.arenas[arena_kind];
    FrontendContextFrame *frame = (FrontendContextFrame *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    if (frame) {
        frame->object = object;
        frame->previous = top;
        frame->saved_origin = owner->origin;
        frame->saved_position = owner->position;
        frame->first = first;
        frame->callback = callback;
    }
    top = frame;
    frontend_bss.saved_context_stack.origin = lexer->initial_object;
    frontend_bss.saved_context_stack.position = lexer->initial_value;
}
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
                if (frontend_data.declaration_mode_index < -1 ||
                    frontend_data.declaration_mode_index >= 10) { __asm { int 3 } }
                mode = FRONTEND_LINKAGE_CONTEXT();
                if (mode == 2 &&
                    (!function_attribute || (context.class_flags & 7) == 3))
                    report_language_diagnostic(1, 190, symbol, member);
                function_attribute = 0x200;
            } else {
                if (frontend_data.declaration_mode_index < -1 ||
                    frontend_data.declaration_mode_index >= 10) { __asm { int 3 } }
                mode = FRONTEND_LINKAGE_CONTEXT();
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
void RECON_THISCALL CompilerLexer::Unread(unsigned token)
{
#ifdef RECON_NATIVE_TEST
    if (unread_count != 0) recon_unsupported("lexer: wider unread stack pending");
#endif
    unread_tokens[unread_count] = token;
    ++unread_count;
}
void RECON_THISCALL CompilerLexer::Trace(unsigned token) { __asm { int 3 } }
int RECON_THISCALL CompilerLexer::Valid(void) { return 1; }
/* Original1300 is a single RET0, shared with inert virtual methods and the
 * approved cdecl1 cleanup alias. Never interpret this or any formal here. */
void RECON_THISCALL CompilerLexer::Reset(void) {}
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
    if (frontend_bss.parser_context_depth >= 2) { __asm { int 3 } }
    context = frontend_bss.parameter_contexts[frontend_bss.parser_context_depth];
    frontend_bss.parameter_contexts[frontend_bss.parser_context_depth] = NULL;
}
CompilerParameterSet *RECON_THISCALL CompilerTranslationUnit::EndParameterScope(void)
{
    if (current->level != 1) return NULL;
    CompilerParameterSet *parameters = ((CompilerAuxiliaryScope *)current)->parameters;
    PopCurrentScope();
    return parameters;
}
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
extern "C" CompilerType *__cdecl qualify_type(CompilerSymbol *symbol, unsigned flags, WORD attributes)
{
    CompilerType value, *type;
    CompilerTypeShape *shape;
    if (symbol) {
        type = ((CompilerIrSymbol *)symbol)->type;
    } else {
        int index = frontend_data.declaration_context_index;
        if (index < 0 || index >= 16) { __asm { int 3 } }
        type = frontend_bss.class_scope_rows[index].type->StripQualifiers();
    }
    memcpy(&value, type, 16);
    shape = create_declarator_shape(1, normalize_shape_attributes(0x2001, (WORD)flags), NULL);
    shape = intern_type_shape(shape);
    value.attributes |= attributes;
    value.shape = shape;
    return intern_type(&value);
}
CompilerType *RECON_THISCALL CompilerType::StripQualifiers(void)
{
    if (shape) {
        if (!(shape->attributes & 0x1f00)) return this;
        CompilerTypeShape value;
        memcpy(&value, shape, 12);
        value.attributes &= 0xf0ff;
        return WithShape(intern_type_shape(&value));
    }
    if (!(attributes & 0x1f00)) return this;
    CompilerType value;
    memcpy(&value, this, 16);
    value.attributes &= 0xf0ff;
    return intern_type(&value);
}
CompilerType *RECON_THISCALL CompilerType::ApplyTypeQualifiers(WORD qualifiers)
{
    CompilerTypeShape *node;
    if (shape) {
        node = shape->Find(&CompilerTypeShape::IsNotArray);
        if (node) {
            if ((node->flags & 0x40) || (node->flags & 0x77) == 4)
                return this;
            CompilerTypeShape value;
            memcpy(&value, node, 12);
            value.attributes |= qualifiers;
            return WithShape(shape->Replace(node, &value));
        }
    }
    CompilerType value;
    memcpy(&value, this, 16);
    value.attributes |= qualifiers;
    return intern_type(&value);
}
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::Find(int (RECON_THISCALL CompilerTypeShape::*)(void)) { __asm { int 3 } return NULL; }
int RECON_THISCALL CompilerTypeShape::IsNotArray(void) { __asm { int 3 } return 0; }
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::Replace(CompilerTypeShape *, CompilerTypeShape *) { __asm { int 3 } return NULL; }
CompilerType *RECON_THISCALL CompilerType::WithShape(CompilerTypeShape *) { __asm { int 3 } return NULL; }
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
    memcpy(&qualifiers, value, 8); /* both actual DWORDs, including the uninterpreted WORD gap */
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

void RECON_THISCALL CompilerDeclarator::AddPointer(CompilerDeclaratorQualifiers *first, CompilerDeclaratorQualifiers *second)
{
    if (first->flags & 0x70) shape_flags |= 1;
    CompilerTypeShape *next = create_type_shape(1, first->flags | second->flags, &first->value);
    if (last_shape) {
        last_shape->base = next;
        last_shape = next;
    } else {
        last_shape = next;
        shape = next;
    }
}
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
            ValidateReferenceShape(copy);
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
void RECON_THISCALL CompilerDeclarator::ValidateContext(void)
{
    if (qualifiers.flags) {
        report_language_diagnostic(1, 229);
        qualifiers.flags = 0;
        qualifiers.value = 0;
    }
}
CompilerType *RECON_THISCALL CompilerDeclarator::BuildType(CompilerTypeShape *shape)
{
    CompilerType description = *base_type;
    description.attributes &= ~0x70;
    description.shape = shape;
    if (shape && (shape->flags & 0x77) == 4)
        description.attributes |= shape->attributes & 0x70;
    return intern_type(&description);
}
CompilerTypeShape *RECON_THISCALL CompilerTypeShape::InternChain(CompilerTypeShape *stop)
{
    if (this == stop) return this;
    base = base->InternChain(stop);
    return intern_type_shape(this);
}
extern "C" CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *shape)
{
    unsigned hash;
    CompilerMappedType **bucket, *entry;
    CompilerPool *pool;
    if (!shape) return NULL;
    hash = (((WORD)shape->payload.value >> 2) +
        ((WORD)(unsigned)shape->base >> 2) + shape->flags + shape->attributes) & 0x3ff;
    bucket = &frontend_data.type_tables.mapped->buckets[hash];
    for (entry = *bucket; entry; entry = entry->next) {
        CompilerTypeShape *candidate = &entry->shape;
        if (shape->base == candidate->base && shape->payload.value == candidate->payload.value &&
            shape->flags == candidate->flags && shape->attributes == candidate->attributes)
            return candidate;
    }
    pool = &frontend_bss.pools[1]; /* actual bf5b0 */
    if (pool->remaining < 16) pool->Refill(16);
    entry = (CompilerMappedType *)pool->cursor;
    pool->cursor += 16;
    pool->remaining -= 16;
    if (entry) memset(&entry->shape, 0, sizeof(entry->shape));
    entry->next = *bucket;
    *bucket = entry;
    entry->shape = *shape;
    return &entry->shape;
}
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
void RECON_THISCALL CompilerDeclarator::ValidateReferenceShape(CompilerTypeShape *next)
{
    unsigned diagnostic = 0;
    if (!shape) return;
    switch (last_shape->flags & 0x77) {
    case 1: diagnostic = 0x210; break;
    case 2: diagnostic = 0xea; break;
    case 0x10: diagnostic = 0x27c; break;
    case 0x40: diagnostic = 0x211; break;
    }
    if (diagnostic) {
        compiler_warning(diagnostic, symbol);
        next->flags = 1;
        flags |= 1;
    }
}

extern "C" CompilerSymbol *__cdecl current_declarator_context(void)
{
    int index = frontend_data.declaration_context_index;
    if (index < 0) return NULL;
    if (index >= 16) { __asm { int 3 } }
    return (CompilerSymbol *)frontend_bss.class_scope_rows[index].symbol;
}

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
extern "C" CompilerParameterTypes *__cdecl intern_parameter_types(CompilerParameterTypes *, unsigned);
extern "C" CompilerParameterTypes *__cdecl collect_parameter_types(unsigned count, CompilerParameterSet *parameters)
{
    CompilerArena *arena = &frontend_bss.arenas[5]; /* actual c65a0 */
    CompilerParameterTypes *types = (CompilerParameterTypes *)arena->cursor;
    CompilerSymbolLink *entry;
    CompilerType **destination;
    unsigned hash = 0;
    arena->cursor += (count * 4 + 11) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    types->count = count;
    destination = types->types;
    for (entry = parameters ? parameters->head : NULL; entry; entry = entry->next) {
        CompilerIrSymbol *symbol = (CompilerIrSymbol *)((BYTE *)entry - offsetof(CompilerNamedSymbol, link));
        *destination++ = symbol->type;
        hash += ((WORD)(unsigned)symbol->type >> 2) & 0xfff;
    }
    return intern_parameter_types(types, hash);
}
extern "C" CompilerParameterTypes *__cdecl intern_parameter_types(CompilerParameterTypes *parameters, unsigned hash)
{
    CompilerTypeSequence **bucket = &frontend_data.type_tables.sequences->buckets[hash & 0x1ff];
    CompilerTypeSequence *entry = *bucket, *previous = NULL, *created;
    CompilerPool *pool = &frontend_bss.pools[2]; /* actual c0498 */
    unsigned bytes = parameters->count * 4 + 12;
    int count = (int)parameters->count;
    while (entry) {
        int other_count = (int)entry->parameters.count;
        if (other_count > count) break;
        if (other_count == count && !memcmp(entry->parameters.types, parameters->types, count * 4))
            return &entry->parameters;
        previous = entry;
        entry = entry->next;
    }
    if (pool->remaining < bytes) pool->Refill(bytes);
    created = (CompilerTypeSequence *)pool->cursor;
    pool->cursor += bytes;
    pool->remaining -= bytes;
#ifdef RECON_NATIVE_TEST
    /* The source+4 word is copied internally by the original but not read
     * by these reached consumers. Only count/pointers join this projection;
     * the omitted word has no zero or byte-equality claim. */
    created->parameters.count = parameters->count;
    memcpy(created->parameters.types, parameters->types, count * 4);
#else
    memcpy(&created->parameters, parameters, parameters->count * 4 + 8);
#endif
    created->next = entry;
    if (previous) previous->next = created;
    else *bucket = created;
    return &created->parameters;
}
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
            /* Current records come from the fresh16-byte arena5 producer;
             * committed-page zeroes survive its field stores and this exact
             * whole-record copy. Other origins/reuse remain unsupported.
             * These are representation bytes, not invented semantic fields. */
            const BYTE *current_bytes = (const BYTE *)function;
            const BYTE *entry_bytes = (const BYTE *)&entry->function;
            if (current_bytes[5] | current_bytes[6] | current_bytes[7] |
                entry_bytes[5] | entry_bytes[6] | entry_bytes[7])
                recon_unsupported("function interner: unsupported representation origin");
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
    memcpy(&created->function, function, 16); /* original copies every representation byte */
    created->next = entry;
    if (previous) previous->next = created;
    else *bucket = created;
    return &created->function;
}

WORD RECON_THISCALL CompilerDeclarator::DefaultCallingConvention(CompilerTypeShape *function)
{
    if (function->flags & 0x8000) {
        if (frontend_data.options.fastcall_calls || frontend_data.options.pascal_calls)
            report_language_diagnostic(2, 96, 37, 106);
        return 0x10;
    }
    if (function->flags & 0x2000) {
        if ((context.class_flags & 7) != 3) {
            CompilerNameEntry *name = ((CompilerNamedSymbol *)symbol)->link.name;
            if (name != frontend_data.builtin_names.operator_new &&
                name != frontend_data.builtin_names.operator_delete &&
                name != frontend_data.builtin_names.operator_new_array &&
                name != frontend_data.builtin_names.operator_delete_array)
                return 0x70;
        }
        if (function != shape) return 0x70;
    }
    if (frontend_data.options.fastcall_calls) return 0x40;
    if (frontend_data.options.stdcall_calls) return 0x60;
    return frontend_data.options.pascal_calls ? 0x20 : 0x10;
}
WORD RECON_THISCALL CompilerDeclarator::PropagateQualifiers(CompilerTypeShape *shape, WORD attributes)
{
    if (shape->flags & 0x8000) {
        switch (attributes & 0x70) {
        case 0x40: case 0x60: return 0x10;
        case 0x10: case 0x50: break;
        default:
            compiler_warning(217, 37, 106);
            return 0x10;
        }
    }
    return attributes;
}

extern "C" WORD __cdecl normalize_shape_attributes(unsigned flags, WORD attributes)
{
    unsigned kind = flags & 0x77;
    WORD convention;
    if (frontend_bss.options.prototypes_only) return attributes;
    switch (kind) {
    case 3: case 4: case 0x44:
        break;
    default:
        return attributes;
    }
    convention = attributes & 0x70;
    if (!convention) {
        if ((flags & 0x8000) &&
            (frontend_data.options.fastcall_calls || frontend_data.options.pascal_calls))
            report_language_diagnostic(2, 96, 37, 106);
        if (flags & 0x8000) attributes |= 0x10;
        else if (frontend_data.options.fastcall_calls) attributes |= 0x40;
        else if (frontend_data.options.stdcall_calls) attributes |= 0x60;
        else if (frontend_data.options.pascal_calls) attributes |= 0x20;
        else attributes |= 0x10;
    } else if ((convention == 0x60 || convention == 0x40) && (flags & 0x8000)) {
        attributes &= ~0x60;
        attributes |= 0x10;
    }
    if (kind == 4 && (flags & 0x8000)) {
        convention = attributes & 0x70;
        if (convention != 0x60 && convention != 0x50 && convention != 0x10)
            compiler_warning(217, 37, 106);
    }
    return attributes;
}
extern "C" CompilerTypeShape *__cdecl create_declarator_shape(unsigned flags, WORD attributes, void *value)
{
    unsigned arena_kind = (frontend_data.compiler_mode_flags & 0x20000000) ? 0 : 5;
    CompilerArena *arena = &frontend_bss.arenas[arena_kind];
    CompilerTypeShape *shape = (CompilerTypeShape *)arena->cursor;
    arena->cursor += 12;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    shape->flags = (WORD)flags;
    shape->attributes = attributes;
    switch (flags & 0x77) {
    case 1: case 0x40:
        if ((attributes & 1) && value) shape->payload.value = *(unsigned *)value;
        break;
    case 2:
        if (flags & 0x1000) shape->payload.value = *(unsigned *)value;
        else if (!value) shape->payload.value = 0;
        else {
            if (*(int *)value < 0) {
                compiler_warning(118);
                *(int *)value = 1;
            }
            shape->payload.value = *(unsigned *)value;
        }
        break;
    case 4:
        if (flags & 0x400) {
            shape->flags &= ~0x400;
            shape->payload.value = *(unsigned *)value;
        } else {
            CompilerParameterSet *parameters = value ? *(CompilerParameterSet **)value : NULL;
            /* The original uses its discarded by-value attributes slot for
             * this temporary flags word. No caller-visible slot is modeled. */
            WORD function_flags = (WORD)flags;
            if (frontend_bss.parser_context_depth >= 2) { __asm { int 3 } }
            shape->payload.function = create_function_parameters(parameters, &function_flags,
                link_parameter_symbols(frontend_bss.parameter_contexts[frontend_bss.parser_context_depth]), NULL);
            if (frontend_bss.parser_context_depth >= 2) { __asm { int 3 } }
            frontend_bss.parameter_contexts[frontend_bss.parser_context_depth] = NULL;
            shape->flags |= function_flags;
        }
        break;
    case 0x10: case 0x20:
        shape->payload.value = *(unsigned *)value;
        break;
    }
    return shape;
}

extern "C" void *__cdecl allocate_symbol_storage(unsigned bytes, unsigned kind, unsigned arena_index)
{
    unsigned index = kind & 0xff;
    CompilerArena *arena;
    BYTE *memory;
    (void)bytes; /* Kept as a genuine ABI argument; the original ignores it. */
    if (index >= 26 || arena_index >= 9) { __asm { int 3 } }
    arena = &frontend_bss.arenas[arena_index];
    memory = arena->cursor;
    arena->cursor += (frontend_data.symbol_record_bytes[index] + 3) & ~3U;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    return memory;
}

/* The original ignores its caller-supplied size and always reserves72 bytes
 * in arena5. No declarator contents are initialized by this allocator. */
extern "C" BYTE *__cdecl allocate_declarator(unsigned)
{
    CompilerArena *arena = &frontend_bss.arenas[5];
    BYTE *storage = arena->cursor;
    arena->cursor += 72;
    if (arena->cursor > arena->committed_limit) arena->CommitExtent();
    return storage;
}

void RECON_THISCALL CompilerDeclarator::AddArrayBound(__int64 *value)
{
    unsigned count;
    /* Only the low signed DWORD is consumed. The original reuses its
     * incoming pointer slot for count, then discards that slot with RET4. */
    memcpy(&count, value, 4);
    if ((LONG)count < 0) {
        compiler_warning(0x76);
        count = 1;
    }
    CompilerTypeShape *next = create_type_shape(2, 0, &count);
    AddArrayShape(next);
    if (!last_shape) {
        last_shape = next;
        shape = next;
    } else {
        last_shape->base = next;
        last_shape = next;
    }
}
