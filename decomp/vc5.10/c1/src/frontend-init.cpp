#include <io.h>
#include <new.h>
#include "frontend.h"
#include "packing.h"
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
    /* The pending initializer owns the unrecovered translation-unit layout. */
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
