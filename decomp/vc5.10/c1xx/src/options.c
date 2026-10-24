#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "frontend.h"
#include "pdb-context.h"
#include <recon.h>

__declspec(dllimport) unsigned long __cdecl SigForPbCb(const void *, long,
                                                       unsigned long);
const char compiler_empty_string[] = "";

static CompilerZRule letter_options[] = {
    { 'a', 5, &frontend_data.options.language_extensions },
    { 'e', 1, &frontend_data.options.language_extensions },
    { 'E', 1, &frontend_data.options.preprocess_only },
    { 'c', 1, &frontend_bss.options.option_zc },
    { 'i', 1, &frontend_data.options.debug_info },
    { '7', 1, &frontend_data.options.debug_old_format },
    { 'n', 1, &frontend_bss.options.option_zn },
    { 'f', 1, &frontend_bss.options.option_zf },
    { 'g', 1, &frontend_bss.options.prototypes_only },
    { 'p', 1, &frontend_data.options.packing_alignment },
    { 'I', 1, &frontend_bss.options.edit_and_continue },
    { 0, 0, NULL }
};

/* Full encountered grammar in original order; repeated spellings share
 * their actual typed destination rather than allocating duplicate flags. */
static CompilerOptionRule option_rules[] = {
    { "@$", &frontend_bss.options.response_file, 1, 0x22 },
    { "-nologo", &frontend_bss.options.suppress_banner, 1, 0x1 },
    { "-FAT", &frontend_bss.options.fat, 1, 0x1 },
    { "-pc#", &frontend_data.options.path_separators, 1, 0x22 },
    { "-pf", &frontend_data.options.option_pf, 1, 0x1 },
    { "-C", &frontend_bss.options.preserve_comments, 1, 0x1 },
    { "-D#", &frontend_bss.options.defines, 1, 0x26 },
    { "-E", &frontend_bss.options.preprocess_stdout, 1, 0x1 },
    { "-I#", &frontend_bss.options.include_directories, 1, 0x26 },
    { "-FI#", &frontend_bss.options.forced_includes, 1, 0x26 },
    { "-EP", &frontend_data.options.preprocess_no_lines, 1, 0x1 },
    { "-Fi#", &frontend_bss.options.preprocessed_file, 1, 0x22 },
    { "-f", &frontend_bss.options.source_file, 1, 0x22 },
    { "-J", &frontend_data.options.unsigned_char, 1, 0x1 },
    { "-Jd", &frontend_data.options.japanese_identifiers, 1, 0x1 },
    { "-Bd%", &frontend_bss.options.dump_options, 1, 0x1 },
    { "-Bu#", &frontend_data.options.option_bu, 1, 0x24 },
    { "-Bm#", &frontend_data.option_bm, 1, 0x24 },
    { "-Zm#", &frontend_bss.options.pch_memory_limit, 1, 0x24 },
    { "-Brepro", &frontend_data.options.reproducible, 1, 0x1 },
    { "-MT", &frontend_data.options.multithread_runtime, 1, 0x1 },
    { "-MD", &frontend_data.options.dll_runtime, 1, 0x1 },
    { "-WX", &frontend_bss.options.warnings_are_errors, 1, 0x1 },
    { "-W#", &frontend_data.options.warning_level, 1, 0x24 },
    { "-Zp", &frontend_data.options.packing_alignment, 1, 0x1 },
    { "-Zp#", &frontend_data.options.packing_alignment, 1, 0x24 },
    { "-ZBe", &frontend_data.options.option_zbe, 1, 0x1 },
    { "-ZB", &frontend_data.option_zb, 1, 0x1 },
    { "-ZB*", &frontend_data.option_zb, 1, 0x24 },
    { "-Z*", letter_options, 1, 0x23 },
    { "-ef#", &frontend_bss.options.error_file, 1, 0x22 },
    { "-il$", &frontend_data.options.il_prefix, 1, 0x22 },
    { "-xc", &frontend_bss.options.option_xc, 1, 0x1 },
    { "-V#", &frontend_bss.options.version_string, 1, 0x22 },
    { "-Gs", &frontend_data.options.stack_checks, 1, 0x5 },
    { "-Gc", &frontend_data.options.pascal_calls, 1, 0x1 },
    { "-Gr", &frontend_data.options.fastcall_calls, 1, 0x1 },
    { "-Gd", &frontend_data.options.cdecl_calls, 1, 0x1 },
    { "-Gz", &frontend_data.options.stdcall_calls, 1, 0x1 },
    { "-Gi", &frontend_data.options.incremental_compile, 1, 0x1 },
    { "-GI", &frontend_data.options.debug_types, 1, 0x1 },
    { "-GFR", &frontend_data.options.function_records, 1, 0x1 },
    { "-GFR-", &frontend_data.options.function_records, 1, 0x5 },
    { "-FD", &frontend_data.options.dependency_output, 1, 0x1 },
    { "-NV#", &frontend_data.options.new_version, 1, 0x22 },
    { "-Fr#", &frontend_bss.options.browser_local_file, 1, 0x22 },
    { "-FR#", &frontend_bss.options.browser_file, 1, 0x22 },
    { "-Fd#", &frontend_data.options.database_file, 1, 0x22 },
    { "-Fo*", &frontend_data.options.object_file, 1, 0x22 },
    { "-Gf", &frontend_data.options.pool_strings, 1, 0x1 },
    { "-GF", &frontend_data.options.readonly_strings, 1, 0x1 },
    { "-cbstring", &frontend_data.options.code_based_strings, 1, 0x1 },
    { "-Oa", &frontend_data.options.assume_no_alias, 1, 0x1 },
    { "-Ob#", &frontend_data.options.inline_level, 1, 0x24 },
    { "-Oi", &frontend_data.options.intrinsics, 1, 0x1 },
    { "-Og", &frontend_data.options.global_optimize, 1, 0x1 },
    { "-Op", &frontend_data.options.precise_float, 1, 0x1 },
    { "-Os", &frontend_data.options.optimize_speed, 1, 0x5 },
    { "-Ot", &frontend_data.options.optimize_speed, 1, 0x1 },
    { "-Ow", &frontend_data.options.assume_no_global_alias, 1, 0x1 },
    { "-Oq", &frontend_data.options.option_oq, 1, 0x1 },
    { "-Ov", &frontend_data.options.option_ov, 1, 0x1 },
    { "-Oy", &frontend_data.options.omit_frame_pointer, 1, 0x1 },
    { "-debugBreak%", &frontend_bss.break_into_debugger, 1, 0x1 },
    { "-GX", &frontend_data.options.exception_handling, 1, 0x1 },
    { "-EHa", &frontend_data.options.exception_handling, 1, 0x1 },
    { "-EHs", &frontend_data.options.exception_handling, 1, 0x1 },
    { "-GR", &frontend_data.options.runtime_type_info, 1, 0x1 },
    { "-Gm", &frontend_data.options.incremental_rebuild, 1, 0x1 },
    { "-vmb", &frontend_data.options.member_pointer_best_case, 1, 0x1 },
    { "-vmg", &frontend_data.options.member_pointer_general, 1, 0x1 },
    { "-vms", &frontend_data.options.member_pointer_single, 1, 0x1 },
    { "-vmm", &frontend_data.options.member_pointer_multiple, 1, 0x1 },
    { "-vmv", &frontend_data.options.member_pointer_virtual, 1, 0x1 },
    { "-vd0", &frontend_data.options.vtordisp, 1, 0x5 },
    { "-vd1", &frontend_data.options.vtordisp, 1, 0x1 },
    { "-Binl", &frontend_bss.options.option_binl, 1, 0x1 },
    { "-EHc", &frontend_data.options.extern_c_no_throw, 1, 0x1 },
    { "-Yu*", &frontend_data.options.pch_use_file, 1, 0x22 },
    { "-Yu", &frontend_data.options.pch_use, 1, 0x1 },
    { "-Yc*", &frontend_data.options.pch_create_file, 1, 0x22 },
    { "-Yc", &frontend_data.options.pch_create, 1, 0x1 },
    { "-Yd", &frontend_data.options.pch_debug, 1, 0x1 },
    { "-Fp#", &frontend_data.options.pch_file, 1, 0x22 },
    { "-YX*", &frontend_data.options.pch_auto_file, 1, 0x22 },
    { "-YX", &frontend_data.options.pch_auto, 1, 0x1 },
    { "-Yl*", &frontend_data.options.pch_link_name, 1, 0x22 },
    { "-BNOPPCH", &frontend_data.options.pch_no_preprocess, 1, 0x5 },
    { "-noexpinl", &frontend_data.options.no_expand_inline, 1, 0x1 },
    { "-GM", &frontend_data.options.minimal_rebuild, 1, 0x1 },
    { "-G#", &frontend_data.options.processor, 1, 0x24 },
    { "-BMOVE", &frontend_data.options.move_pch, 1, 0x1 },
    { "-C9IL", &frontend_data.option_c9il, 1, 0x1 },
    { "-SplitPdbs", &frontend_data.options.split_pdbs, 1, 0x1 },
    { "-SplitPdbs-", &frontend_data.options.split_pdbs, 1, 0x5 },
    { "-NoEHForNew", &frontend_data.options.no_eh_for_new, 1, 0x1 },
    { "-Bshow_incl", &frontend_bss.options.show_includes, 1, 0x1 },
    { "-Bcmpconv", &frontend_data.options.option_bcmpconv, 1, 0x1 },
    { "-noBool", &frontend_data.options.no_bool, 1, 0x1 },
    { "*", &frontend_data.options.unrecognized_argument, 0, 0x22 },
    { NULL, NULL, 0, 0 }
};

typedef struct WarningDefault {
    unsigned code;
    unsigned char flags;
} WarningDefault;

static WarningDefault warning_defaults[] = {
    { 1, 5 },
    { 69, 5 },
    { 611, 5 },
    { 226, 14 },
    { 233, 14 },
    { 234, 14 },
    { 235, 14 },
    { 168, 5 },
    { 169, 5 },
    { 530, 5 },
    { 0, 0 }
};

void push_declaration_context(int context);
void initialize_include_aliases(void);
void initialize_checksums(void);
void initialize_multibyte_categories(void);
void initialize_lowercase_table(void);
void read_env_arguments(char *arguments);
char *duplicate_argument(const char *argument, int copy);
int report_response_error(int number, ...);
void stop_compiler(int status, int detail);
struct CompilerVector *new_option_vector(unsigned capacity, int arena_index);
char *next_argument(void);
int match_option(CompilerOptionRule *rules, char *argument,
                 char *(__cdecl *next)(void), int copy);
char *resolve_option_path(char *output, const char *input, unsigned capacity);
void disable_multibyte_identifiers(void);
void initialize_packing(void);
int compare_pch_paths(const char *left, const char *right);
void free_argument_allocation(void *allocation);

/* Original hot38fad..39282 and cold7217a..7240b. The initial declaration
 * context call dominates every option read and remains the first named trap. */
void process_compiler_options(int argc, char **argv)
{
    char resolved[520], response_line[2048];
    char *line;
    FILE *response;
    WarningDefault *warning;
    unsigned length;

    push_declaration_context(0);
    if (!frontend_data.options.include_aliases)
        initialize_include_aliases();
    initialize_checksums();
    setlocale(LC_CTYPE, compiler_empty_string);
    initialize_multibyte_categories();
    initialize_lowercase_table();

    if (argc > 1 && argv[1][0] == '@') {
        response = fopen(argv[1] + 1, "rt");
        if (!response) {
            report_response_error(84, 0x151, argv[1] + 1);
            stop_compiler(1, 0);
        }
        line = response_line;
        while (fgets(response_line, 2048, response)) {
            if (response_line[0] != ';') {
                for (length = 0; response_line[length]; ++length)
                    ;
                --length;
                if (response_line[length] == '\n')
                    response_line[length] = 0;
                if (response_line[0] == 'P' && response_line[1] == '1' &&
                    response_line[2] == ':')
                    line = response_line + 3;
                read_env_arguments(duplicate_argument(line, 1));
            }
            line = response_line;
        }
        fclose(response);
    }
    read_env_arguments(getenv("MSC_CMD_FLAGS"));
    for (warning = warning_defaults; warning->code; ++warning) {
        frontend_data.options.warning_levels[warning->code] = warning->flags;
        if (warning->code >= 700 && warning->code < 1000) {
            if (frontend_data.options.lowest_warning >= warning->code)
                frontend_data.options.lowest_warning = warning->code;
            if (frontend_data.options.highest_warning <= warning->code)
                frontend_data.options.highest_warning = warning->code;
        }
    }
    frontend_bss.options.defines = new_option_vector(40, 1);
    frontend_bss.options.include_directories = new_option_vector(40, 1);
    frontend_bss.options.forced_includes = new_option_vector(40, 1);
    if (!frontend_data.option_c9il)
        frontend_bss.source_signature = SigForPbCb("CPP", 3,
                                                  frontend_bss.source_signature);
    while (match_option(option_rules, next_argument(), next_argument, 0))
        ;
    if (!resolve_option_path(resolved, frontend_data.options.database_file, 520))
        report_compiler_diagnostic(83, 0x162,
                                   frontend_data.options.database_file, 77, 0x163);
    for (length = 0; resolved[length]; ++length)
        ;
    if (length >= 256)
        report_compiler_error(5);
    for (length = 0;
         (frontend_data.options.database_path[length] = resolved[length]) != 0;
         ++length)
        ;
    if (frontend_data.options.debug_old_format) {
        frontend_data.options.debug_info = 1;
        frontend_data.options.line_records = 0;
    }
    frontend_data.option_c9il |= frontend_data.options.incremental_compile == 0;
    frontend_data.options.function_records &=
        frontend_data.options.incremental_compile;
    current_pdb_context = frontend_data.options.split_pdbs ?
        &output_pdb_context : &input_pdb_context;
    if (frontend_data.options.incremental_rebuild) {
        if (!frontend_data.options.incremental_compile) {
            frontend_data.options.incremental_rebuild_forced = 1;
            frontend_data.options.incremental_compile = 1;
            frontend_data.options.debug_types = 1;
        }
        frontend_data.options.line_records = 1;
    }
    frontend_data.options.line_records &= frontend_data.options.debug_info;
    if (frontend_data.options.japanese_identifiers)
        disable_multibyte_identifiers();
    if (!frontend_data.options.packing_stack)
        initialize_packing();
    if (frontend_data.options.pch_link_name) {
        for (length = 0; frontend_data.options.pch_link_name[length]; ++length)
            ;
        if (length >= 256)
            report_compiler_error(5);
        for (length = 0; (frontend_data.options.pch_link_path[length] =
                         frontend_data.options.pch_link_name[length]) != 0;
             ++length)
            ;
    }
    if (frontend_data.options.unrecognized_argument)
        report_compiler_diagnostic(7,
                                   frontend_data.options.unrecognized_argument,
                                   "c1");
    if (!frontend_bss.options.source_file)
        report_compiler_error(8);
    if (frontend_data.options.pch_create_file) {
        frontend_data.options.pch_create = 1;
        if (frontend_data.options.pch_use_file &&
            compare_pch_paths(frontend_data.options.pch_create_file,
                              frontend_data.options.pch_use_file)) {
            frontend_data.options.pch_use = 0;
            frontend_data.options.pch_use_file = NULL;
        }
    }
    if (!frontend_data.options.pch_create)
        frontend_data.options.pch_no_preprocess = 0;
    if (frontend_data.options.pch_use_file)
        frontend_data.options.pch_use = 1;
    if (frontend_data.options.pch_auto_file)
        frontend_data.options.pch_auto = 1;
    frontend_data.options.pch_debug |= frontend_data.options.pch_auto;
    if (frontend_bss.options.preprocess_stdout ||
        frontend_bss.options.prototypes_only) {
        frontend_data.options.pch_auto = 0;
        frontend_data.options.pch_auto_file = NULL;
        frontend_data.options.pch_create = 0;
        frontend_data.options.pch_create_file = NULL;
        frontend_data.options.pch_use = 0;
        frontend_data.options.pch_use_file = NULL;
        frontend_data.options.pch_debug = 0;
        frontend_data.options.pch_no_preprocess = 0;
    }
    if (frontend_data.options.argument_allocation) {
        free_argument_allocation(frontend_data.options.argument_allocation);
        frontend_data.options.argument_allocation = NULL;
    }
}

/* Linkage context values are scalar: consumers compare2 ("C" linkage),
 * and the matching pop only decrements depth. No pointee is implied. */
void push_declaration_context(int context)
{
    if (++frontend_data.language_context_depth == 10)
        report_compiler_error(45);
    frontend_bss.language_contexts[frontend_data.language_context_depth] = context;
}
void initialize_include_aliases(void) { __asm { int 3 } }
void initialize_checksums(void) { __asm { int 3 } }
void initialize_multibyte_categories(void) { __asm { int 3 } }
void initialize_lowercase_table(void) { __asm { int 3 } }
void read_env_arguments(char *arguments) { __asm { int 3 } }
char *duplicate_argument(const char *argument, int copy) { __asm { int 3 } return NULL; }
int report_response_error(int number, ...) { __asm { int 3 } return 0; }
void stop_compiler(int status, int detail) { __asm { int 3 } }
struct CompilerVector *new_option_vector(unsigned capacity, int arena_index) { __asm { int 3 } return NULL; }
char *next_argument(void) { __asm { int 3 } return NULL; }
int match_option(CompilerOptionRule *rules, char *argument, char *(__cdecl *next)(void), int copy) { __asm { int 3 } return 0; }
char *resolve_option_path(char *output, const char *input, unsigned capacity) { __asm { int 3 } return NULL; }
void disable_multibyte_identifiers(void) { __asm { int 3 } }
void initialize_packing(void) { __asm { int 3 } }
int compare_pch_paths(const char *left, const char *right) { __asm { int 3 } return 0; }
void free_argument_allocation(void *allocation) { __asm { int 3 } }
