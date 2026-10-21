/* Frontend state, keyword exclusions, and builtin types initialized at 0x68ce0. */
#include <windows.h>
#include <io.h>
#include "initialization.h"

WorkingStack *compiler_stack;
int builtin_type_context;
int pch_separate_debug, pch_read_enabled, pch_write_enabled;
int data_pointer_bytes, code_pointer_bytes;
int data_pointer_alignment, code_pointer_alignment;
int data_pointer_kind, code_pointer_kind;
int data_pointer_model, code_pointer_model;
int normalized_Op, normalized_Oy, normalized_Oa, normalized_Ow, normalized_Oz;
int normalized_Gs, normalized_PLMN, normalized_PLMF, normalized_Zr;
int normalized_Oq, normalized_Gn, normalized_Ol;
int target_cpu, target_cpu_mask;
char token_buffer[7936];
char *source_filename = token_buffer, *source_buffer_cursor = token_buffer;
char *browser_filename;
FILE *diagnostic_trace_file, *listing_file, *preprocessor_stream;
CType *type_0000, *type_4041, *type_0064;
CType *type_4064_first, *type_4064_second;
CType *type_0042, *type_0044, *type_4044;
CType *type_0108, *type_0802, *type_0080;
CType *type_0041_q1000, *type_4041_q1000, *type_0064_q1000;
CType *type_implicit_int_function, *type_tagged_0080;
CType *type_indirect_0080_2, *type_indirect_0080_1, *type_4041_q1;

/* Original 0x68ce0..0x69621. */
void initialize_frontend(void)
{
    PackingEntry *packing;
    ArenaPool *pool;
    TypeDescription *description;
    SetHandleCount(30);
    initialize_work_stack(&compiler_stack);
    if (options.pch_auto_file)
        options.pch_auto = 1;
    if (options.option_E) {
        options.configuration.option_Yd = 0;
        options.pch_create = 0;
        options.pch_create_file = 0;
        options.pch_auto = 0;
    } else
        options.configuration.option_Yd |= (signed char)options.pch_auto;
    pch_separate_debug = options.configuration.debug_info && !options.configuration.option_Yd &&
                         (options.pch_create || options.pch_create_file);
    if (options.pch_create_file && options.pch_use_file &&
        compare_filenames(options.pch_create_file, options.pch_use_file))
        options.pch_use_file = 0;
    pch_read_enabled = options.pch_use || options.pch_use_file;
    pch_write_enabled = options.pch_create || options.pch_create_file;
    initialize_preprocessor();
    initialize_keyword_table();
    initialize_floating_point();
    code_pointer_bytes = code_pointer_alignment = 4;
    if (options.configuration.option_M & 1)
        data_pointer_kind = data_pointer_model = 2;
    else
        data_pointer_kind = data_pointer_model = 1;
    if (options.configuration.option_M & 2) {
        data_pointer_bytes = data_pointer_alignment = 8;
        code_pointer_kind = code_pointer_model = 2;
    } else {
        data_pointer_bytes = data_pointer_alignment = 4;
        code_pointer_kind = code_pointer_model = 1;
    }
    options.configuration.option_EP = options.configuration.option_EP || !options.option_E;
    packing = top_packing(packing_stack);
    packing->alignment = options.option_Zp == -1 ? 8 : options.option_Zp;
    normalized_Op = options.configuration.option_Op;
    normalized_Oy = options.configuration.option_Oy;
    normalized_Oa = options.configuration.option_Oa;
    normalized_Ow = options.configuration.option_Ow;
    normalized_Oz = options.configuration.option_Oz;
    normalized_Gs = options.configuration.option_Gs;
    normalized_PLMN = options.configuration.option_PLMN;
    normalized_PLMF = options.configuration.option_PLMF;
    normalized_Zr = options.z_r;
    if (options.configuration.option_qc && options.option_Oe)
        options.configuration.option_Ol = 1;
    normalized_Oq = options.configuration.option_Oq;
    normalized_Gn = options.configuration.option_Gn;
    target_cpu = 0;
    if (options.configuration.option_G1)
        target_cpu = 1;
    else if (options.configuration.option_G2)
        target_cpu = 2;
    else if (options.configuration.option_G4)
        target_cpu = 4;
    else {
        target_cpu = 3;
        options.configuration.option_G3 = 1;
    }
    target_cpu_mask = 1 << (unsigned char)target_cpu;
    options.configuration.option_G2 = options.configuration.option_G1 || options.configuration.option_G2 || options.configuration.option_G3 || options.configuration.option_G4;
    if (!options.z_f) {
        disable_keyword("fortran");
        disable_keyword("__fortran");
        if (options.configuration.z_a || options.configuration.z_E)
            disable_keyword("_fortran");
        if (!options.option_m) {
            disable_keyword("pascal");
            disable_keyword("__pascal");
            if (options.configuration.z_a || options.configuration.z_E)
                disable_keyword("_pascal");
        }
        disable_keyword("near");
        disable_keyword("__near");
        if (options.configuration.z_a || options.configuration.z_E)
            disable_keyword("_near");
        disable_keyword("far");
        disable_keyword("__far");
        if (options.configuration.z_a || options.configuration.z_E)
            disable_keyword("_far");
        disable_keyword("huge");
        disable_keyword("__huge");
        if (options.configuration.z_a || options.configuration.z_E)
            disable_keyword("_huge");
        disable_keyword("__far16");
        if (options.configuration.z_a || options.configuration.z_E)
            disable_keyword("_far16");
        disable_keyword("__syscall");
        if (options.configuration.z_a || options.configuration.z_E)
            disable_keyword("_syscall");
    }
    disable_keyword("__export");
    if (options.configuration.z_a || options.configuration.z_E)
        disable_keyword("_export");
    if (!options.configuration.z_a) {
        if (!options.configuration.z_E) {
            disable_keyword("__asm");
            if (options.configuration.z_a || options.configuration.z_E)
                disable_keyword("_asm");
        }
        disable_keyword("cdecl");
        disable_keyword("near");
    }
    source_filename = options.source_file;
    do {
        *source_buffer_cursor++ = *source_filename++;
    } while (source_buffer_cursor[-1]);
    source_filename = token_buffer;
    initialize_cpp_context_stack();
    if (options.option_FR) {
        options.option_Fr = options.option_FR;
        options.configuration.browser_full = 1;
    }
    if (options.option_Fr) {
        options.configuration.browser_enabled = 1;
        begin_browser_output();
    }
    browser_filename = options.option_Fr;
    if (options.option_Fs && !options.syntax_only) {
        diagnostic_trace_file = open_intermediate(options.intermediate_prefix, "er", "w");
        listing_file = open_intermediate(options.intermediate_prefix, "lp", "w");
    }
    if (!options.option_E && !options.syntax_only) {
        symbol_file = open_intermediate(options.intermediate_prefix, "sy", "w+");
        global_file = open_intermediate(options.intermediate_prefix, "gl", "w+");
        open_expression_intermediate();
        line_file = open_intermediate(options.intermediate_prefix, "in", "w+");
        if (options.configuration.debug_info)
            debug_file = open_intermediate(options.intermediate_prefix, "db", "w+");
    }
    if (options.configuration.option_Oi && options.configuration.option_qc)
        options.configuration.option_Oi = 0;
    if (options.syntax_only) {
        options.configuration.option_Ol = 0;
        normalized_Ol = 0;
    }
    normalized_Ol = options.configuration.option_Ol;
    if (normalized_Ol && options.configuration.option_qc)
        initialize_inline_queries();
    if (options.option_Fi) {
        if (_dup(1) == -1 ||
            !(preprocessor_file = freopen(options.option_Fi, "wt+", stdout)))
            report_fatal(83, message_text(340), options.option_Fi);
    }
    preprocessor_stream = stdout;
    initialize_type_table();
    if (!options.option_E) {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(TypeDescription))
            refill_arena(&compiler_arena, pool);
        description = (TypeDescription *)pool->next;
        pool->remaining -= sizeof(TypeDescription);
        pool->next += sizeof(TypeDescription);
        initialize_global_scope();
        description->attributes = 0;
        description->related = 0;
        description->field08 = 0;
        initialize_declaration_table();
        builtin_type_context = 1;
        description->flags = 0;
        type_0000 = intern_type(description);
        description->flags = 0x4041;
        type_4041 = intern_type(description);
        description->flags = 0x64;
        type_0064 = intern_type(description);
        description->flags = 0x4064;
        type_4064_first = intern_type(description);
        description->flags = 0x4064;
        type_4064_second = intern_type(description);
        description->flags = 0x42;
        type_0042 = intern_type(description);
        description->flags = 0x44;
        type_0044 = intern_type(description);
        description->flags = 0x4044;
        type_4044 = intern_type(description);
        description->flags = 0x108;
        type_0108 = intern_type(description);
        description->flags = 0x802;
        type_0802 = intern_type(description);
        description->flags = 0x80;
        type_0080 = intern_type(description);
        description->attributes = 0x1000;
        description->flags = 0x41;
        type_0041_q1000 = intern_type(description);
        description->flags = 0x4041;
        type_4041_q1000 = intern_type(description);
        description->flags = 0x64;
        type_0064_q1000 = intern_type(description);
        description->attributes = 0;
        type_implicit_int_function = make_function_type(0, type_0064);
        type_tagged_0080 = make_tagged_type(0x80);
        type_indirect_0080_2 = make_qualified_type(type_0080, 1, normalize_type_attributes(1, 2), 0);
        type_indirect_0080_1 = make_qualified_type(type_0080, 1, normalize_type_attributes(1, 1), 0);
        description->flags = 0x4041;
        description->attributes = 1;
        type_4041_q1 = intern_type(description);
        if (!options.syntax_only)
            initialize_intermediate_output();
        initialize_translation_state();
    }
    define_command_line_macros(options.defines);
    if (!options.option_E) {
        initialize_operators();
        if (!pch_read_enabled) {
            declare_runtime_helpers();
            initialize_purecall();
        }
    }
}
