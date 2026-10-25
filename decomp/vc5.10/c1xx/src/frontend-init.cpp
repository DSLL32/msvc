#include <io.h>
#include <new.h>
#include "frontend-init.h"
#include "pointer-array.h"
#include "packing.h"
#include <recon.h>

/* Preserve the observed direct FILE-array import, not SDK __p__iob. */
#if defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
#undef _iob
#undef stdout
extern "C" __declspec(dllimport) FILE _iob[];
#define stdout (&_iob[1])
#endif

/* Identity only: actual name-record fields remain behind Initialize. */
struct CompilerBuiltinName {};
static CompilerBuiltinName builtin_bool_name;

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
void __cdecl initialize_default_function_type(void);
void __cdecl initialize_language_symbols(void);
void __cdecl initialize_scope_symbols(void);
CompilerSymbol *__cdecl lookup_builtin(struct CompilerScope *, CompilerLookup *);
void __cdecl declare_builtin(CompilerDeclaration *, CompilerSymbol *);
int __cdecl report_response_error(int, ...);
}

extern "C" void __cdecl initialize_frontend(void)
{
    CompilerArena *arena;
    CompilerPool *pool;
    CompilerType *description;
    CompilerBuiltinTypes *types = &frontend_data.builtin_types;
    CompilerLexer *lexer;
    CompilerLexerBackend *backend;
    CompilerLookup lookup;
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
    /* Upper auxiliary WORD and final DWORD are not initialized here. */
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
        initialize_default_function_type();
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
                lookup.Initialize(types->type_0060, &builtin_bool_name);
                lookup.Resolve();
                declare_builtin(&declaration,
                    lookup_builtin(frontend_data.current_scope, &lookup));
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
extern "C" void __cdecl initialize_type_table(void) { __asm { int 3 } }
extern "C" CompilerType *__cdecl intern_type(CompilerType *description) { __asm { int 3 } return NULL; }
extern "C" void __cdecl initialize_global_scope(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_builtin_symbols(void) { __asm { int 3 } }
extern "C" CompilerType *__cdecl make_function_type(unsigned attributes, CompilerType *base) { __asm { int 3 } return NULL; }
extern "C" CompilerType *__cdecl make_tagged_type(unsigned flags) { __asm { int 3 } return NULL; }
extern "C" void __cdecl initialize_intermediate_output(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_translation_state(void) { __asm { int 3 } }
extern "C" void __cdecl define_command_line_macros(CompilerVector *definitions) { __asm { int 3 } }
extern "C" void __cdecl begin_line_output(void) { __asm { int 3 } }
extern "C" void __cdecl begin_source_input(void) { __asm { int 3 } }
CompilerTranslationUnit *RECON_THISCALL CompilerTranslationUnit::Initialize(void) { __asm { int 3 } return this; }
int RECON_THISCALL OutputBuffer::Open(const char *prefix, const char *suffix, const char *mode) { __asm { int 3 } return 0; }

extern "C" void __cdecl initialize_first_token(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_language_symbols(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_scope_symbols(void) { __asm { int 3 } }
extern "C" CompilerSymbol *__cdecl lookup_builtin(struct CompilerScope *scope, CompilerLookup *lookup) { __asm { int 3 } return NULL; }
extern "C" void __cdecl declare_builtin(CompilerDeclaration *declaration, CompilerSymbol *symbol) { __asm { int 3 } }
CompilerContextBuffer *RECON_THISCALL CompilerContextBuffer::Initialize(unsigned arena_kind) { __asm { int 3 } return this; }
void RECON_THISCALL FrontendContextStack::Push(void *object, unsigned arena, int first, int second, FrontendContextStack *owner) { __asm { int 3 } }
CompilerLookup *RECON_THISCALL CompilerLookup::Initialize(CompilerType *type, const void *name) { __asm { int 3 } return this; }
void RECON_THISCALL CompilerLookup::Resolve(void) { __asm { int 3 } }
unsigned RECON_THISCALL CompilerLexer::Read(void) { __asm { int 3 } return 0; }
unsigned RECON_THISCALL CompilerLexer::Peek(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Unread(unsigned token) { __asm { int 3 } }
void RECON_THISCALL CompilerLexer::Trace(unsigned token) { __asm { int 3 } }
int RECON_THISCALL CompilerLexer::Valid(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerLexer::Reset(void) { __asm { int 3 } }
void *RECON_THISCALL CompilerLexer::Destroy(unsigned flags) { __asm { int 3 } return this; }

extern "C" void __cdecl initialize_default_function_type(void) { __asm { int 3 } }
