#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "compiler-state.h"
#include <recon.h>

__declspec(dllimport) unsigned long __cdecl SigForPbCb(const void *, long,
                                                       unsigned long);

static CompilerZRule z_rules[] = {
    { 'a', 5, &c1_data.options.language_extensions },
    { 'e', 1, &c1_data.options.language_extensions },
    { 'E', 1, &c1_data.options.preprocess_only },
    { 'c', 1, &c1_bss.options.option_zc },
    { 'i', 1, &c1_data.options.debug_info },
    { '7', 1, &c1_data.options.debug_old_format },
    { 'n', 1, &c1_bss.options.option_zn },
    { 'f', 1, &c1_bss.options.option_zf },
    { 'g', 1, &c1_bss.options.prototypes_only },
    { 'p', 1, &c1_data.options.packing_alignment },
    { 'I', 1, &c1_bss.options.edit_and_continue },
    { 0, 0, NULL }
};

/* Full encountered grammar, in its original order. An option writes its
 * actual destination; repeated spellings deliberately share that storage. */
static CompilerOptionRule option_rules[] = {
    { "@$", &c1_bss.options.response_file, 1, 0x22 },
    { "-nologo", &c1_bss.options.suppress_banner, 1, 1 },
    { "-FAT", &c1_bss.options.fat, 1, 1 },
    { "-pc#", &c1_data.options.path_separators, 1, 0x22 },
    { "-pf", &c1_data.options.option_pf, 1, 1 },
    { "-C", &c1_bss.options.preserve_comments, 1, 1 },
    { "-D#", &c1_bss.options.defines, 1, 0x26 },
    { "-E", &c1_bss.options.preprocess_stdout, 1, 1 },
    { "-I#", &c1_bss.options.include_directories, 1, 0x26 },
    { "-FI#", &c1_bss.options.forced_includes, 1, 0x26 },
    { "-EP", &c1_data.options.preprocess_no_lines, 1, 1 },
    { "-Fi#", &c1_bss.options.preprocessed_file, 1, 0x22 },
    { "-f", &c1_bss.options.source_file, 1, 0x22 },
    { "-J", &c1_data.options.unsigned_char, 1, 1 },
    { "-Jd", &c1_data.options.japanese_identifiers, 1, 1 },
    { "-Bd%", &c1_bss.options.dump_options, 1, 1 },
    { "-Bu#", &c1_data.options.option_bu, 1, 0x24 },
    { "-Bm#", &c1_data.internal_bm_flags, 1, 0x24 },
    { "-Zm#", &c1_bss.options.pch_memory_limit, 1, 0x24 },
    { "-Brepro", &c1_data.options.reproducible, 1, 1 },
    { "-MT", &c1_data.options.multithread_runtime, 1, 1 },
    { "-MD", &c1_data.options.dll_runtime, 1, 1 },
    { "-WX", &c1_bss.options.warnings_are_errors, 1, 1 },
    { "-W#", &c1_data.options.warning_level, 1, 0x24 },
    { "-Zp", &c1_data.options.packing_alignment, 1, 1 },
    { "-Zp#", &c1_data.options.packing_alignment, 1, 0x24 },
    { "-ZBe", &c1_data.options.option_zbe, 1, 1 },
    { "-ZB", &c1_data.internal_zb_value, 1, 1 },
    { "-ZB*", &c1_data.internal_zb_value, 1, 0x24 },
    { "-Z*", z_rules, 1, 0x23 },
    { "-ef#", &c1_bss.options.error_file, 1, 0x22 },
    { "-il$", &c1_data.options.il_prefix, 1, 0x22 },
    { "-xc", &c1_bss.options.option_xc, 1, 1 },
    { "-V#", &c1_bss.options.version_string, 1, 0x22 },
    { "-Gs", &c1_data.options.stack_checks, 1, 5 },
    { "-Gc", &c1_data.options.pascal_calls, 1, 1 },
    { "-Gr", &c1_data.options.fastcall_calls, 1, 1 },
    { "-Gd", &c1_data.options.cdecl_calls, 1, 1 },
    { "-Gz", &c1_data.options.stdcall_calls, 1, 1 },
    { "-Gi", &c1_data.options.incremental_compile, 1, 1 },
    { "-GI", &c1_data.options.debug_types, 1, 1 },
    { "-GFR", &c1_data.options.function_records, 1, 1 },
    { "-GFR-", &c1_data.options.function_records, 1, 5 },
    { "-FD", &c1_data.options.dependency_output, 1, 1 },
    { "-NV#", &c1_data.options.new_version, 1, 0x22 },
    { "-Fr#", &c1_bss.options.browser_local_file, 1, 0x22 },
    { "-FR#", &c1_bss.options.browser_file, 1, 0x22 },
    { "-Fd#", &c1_data.options.database_file, 1, 0x22 },
    { "-Fo*", &c1_data.options.object_file, 1, 0x22 },
    { "-Gf", &c1_data.options.pool_strings, 1, 1 },
    { "-GF", &c1_data.options.readonly_strings, 1, 1 },
    { "-cbstring", &c1_data.options.code_based_strings, 1, 1 },
    { "-Oa", &c1_data.options.assume_no_alias, 1, 1 },
    { "-Ob#", &c1_data.options.inline_level, 1, 0x24 },
    { "-Oi", &c1_data.options.intrinsics, 1, 1 },
    { "-Og", &c1_data.options.global_optimize, 1, 1 },
    { "-Op", &c1_data.options.precise_float, 1, 1 },
    { "-Os", &c1_data.options.optimize_size, 1, 5 },
    { "-Ot", &c1_data.options.optimize_size, 1, 1 },
    { "-Ow", &c1_data.options.assume_no_global_alias, 1, 1 },
    { "-Oq", &c1_data.options.option_oq, 1, 1 },
    { "-Ov", &c1_data.options.option_ov, 1, 1 },
    { "-Oy", &c1_data.options.omit_frame_pointer, 1, 1 },
    { "-debugBreak%", &c1_bss.debug_break, 1, 1 },
    { "-Yu*", &c1_data.options.pch_use_file, 1, 0x22 },
    { "-Yu", &c1_data.options.pch_use, 1, 1 },
    { "-Yc*", &c1_data.options.pch_create_file, 1, 0x22 },
    { "-Yc", &c1_data.options.pch_create, 1, 1 },
    { "-Yd", &c1_data.options.pch_debug, 1, 1 },
    { "-Fp#", &c1_data.options.pch_file, 1, 0x22 },
    { "-YX*", &c1_data.options.pch_auto_file, 1, 0x22 },
    { "-YX", &c1_data.options.pch_auto, 1, 1 },
    { "-Yl*", &c1_data.options.pch_link_name, 1, 0x22 },
    { "-BNOPPCH", &c1_data.options.pch_no_preprocess, 1, 5 },
    { "-noexpinl", &c1_data.options.no_expand_inline, 1, 1 },
    { "-GM", &c1_data.options.minimal_rebuild, 1, 1 },
    { "-G#", &c1_data.options.processor, 1, 0x24 },
    { "-BMOVE", &c1_data.options.move_pch, 1, 1 },
    { "-C9IL", &c1_data.c9_il, 1, 1 },
    { "-SplitPdbs", &c1_data.options.split_pdbs, 1, 1 },
    { "-SplitPdbs-", &c1_data.options.split_pdbs, 1, 5 },
    { "-NoEHForNew", &c1_data.options.no_eh_for_new, 1, 1 },
    { "-Bshow_incl", &c1_bss.options.show_includes, 1, 1 },
    { "*", &c1_data.options.unrecognized_argument, 0, 0x22 },
    { NULL, NULL, 0, 0 }
};

typedef struct WarningDefault {
    unsigned code;
    unsigned char flags;
} WarningDefault;

static WarningDefault warning_defaults[] = {
    { 1, 5 }, { 69, 5 }, { 611, 5 },
    { 226, 14 }, { 233, 14 }, { 234, 14 }, { 235, 14 },
    { 168, 5 }, { 169, 5 }, { 530, 5 }, { 0, 0 }
};

void __cdecl initialize_include_aliases(void);
void __cdecl initialize_checksums(void);
void __cdecl initialize_option_input_12f08(void);
void __cdecl initialize_option_input_12f3e(void);
void __cdecl read_env_arguments(char *);
char *__cdecl duplicate_argument(const char *, int);
void __cdecl report_response_error(int, ...);
void __cdecl stop_compiler(int, int);
struct CompilerVector *__cdecl new_option_vector(unsigned, int);
char *__cdecl next_argument(void);
int __cdecl match_option(CompilerOptionRule *, char *, char *(__cdecl *)(void), int);
char *__cdecl resolve_option_path(char *, const char *, unsigned);
void __cdecl disable_multibyte_identifiers(void);
void __cdecl initialize_packing(void);
int __cdecl compare_pch_paths(const char *, const char *);
void __cdecl free_argument_allocation(void *);

/* Original hot12b49..12df6 and cold3052b..307b1. Unknown callees remain
 * separate named traps; every option destination has its actual lifetime. */
void __cdecl parse_command_line(int argc, char **argv)
{
    char resolved[520], response_line[2048];
    char *line;
    FILE *response;
    WarningDefault *warning;
    unsigned length;

    if (!c1_data.options.include_aliases)
        initialize_include_aliases();
    initialize_checksums();
    setlocale(LC_CTYPE, c1_empty_string);
    initialize_option_input_12f08();
    initialize_option_input_12f3e();

    if (argc > 1 && argv[1][0] == '@') {
        response = fopen(argv[1] + 1, "r");
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
        c1_data.options.warning_levels[warning->code] = warning->flags;
        if (warning->code >= 700 && warning->code < 1000) {
            if (c1_data.options.lowest_warning >= warning->code)
                c1_data.options.lowest_warning = warning->code;
            if (c1_data.options.highest_warning <= warning->code)
                c1_data.options.highest_warning = warning->code;
        }
    }
    c1_bss.options.defines = new_option_vector(40, 1);
    c1_bss.options.include_directories = new_option_vector(40, 1);
    c1_bss.options.forced_includes = new_option_vector(40, 1);
    if (!c1_data.c9_il)
        c1_bss.pdb_signature = SigForPbCb("C", 1, c1_bss.pdb_signature);
    while (match_option(option_rules, next_argument(), next_argument, 0))
        ;
    if (!resolve_option_path(resolved, c1_data.options.database_file, 520))
        compiler_error(83, 0x162, c1_data.options.database_file, 77, 0x163);
    for (length = 0; resolved[length]; ++length)
        ;
    if (length >= 256)
        compiler_fatal(5);
    for (length = 0; (c1_data.options.database_path[length] = resolved[length]) != 0;
         ++length)
        ;
    if (c1_data.options.debug_old_format) {
        c1_data.options.debug_info = 1;
        c1_data.options.line_records = 0;
    }
    c1_data.c9_il |= c1_data.options.incremental_compile == 0;
    c1_data.options.function_records &= c1_data.options.incremental_compile;
    c1_current_pdb_context = c1_data.options.split_pdbs ?
        &c1_output_pdb_context : &c1_pdb_context;
    c1_data.options.line_records &= c1_data.options.debug_info;
    if (c1_data.options.japanese_identifiers)
        disable_multibyte_identifiers();
    if (!c1_data.options.packing_stack)
        initialize_packing();
    if (c1_data.options.pch_link_name) {
        for (length = 0; c1_data.options.pch_link_name[length]; ++length)
            ;
        if (length >= 256)
            compiler_fatal(5);
        for (length = 0; (c1_data.options.pch_link_path[length] =
                         c1_data.options.pch_link_name[length]) != 0; ++length)
            ;
    }
    if (c1_data.options.unrecognized_argument)
        compiler_error(7, c1_data.options.unrecognized_argument, "c1");
    if (!c1_bss.options.source_file)
        compiler_fatal(8);
    if (c1_data.options.pch_create_file) {
        c1_data.options.pch_create = 1;
        if (c1_data.options.pch_use_file &&
            compare_pch_paths(c1_data.options.pch_create_file,
                              c1_data.options.pch_use_file)) {
            c1_data.options.pch_use = 0;
            c1_data.options.pch_use_file = NULL;
        }
    }
    if (!c1_data.options.pch_create)
        c1_data.options.pch_no_preprocess = 0;
    if (c1_data.options.pch_use_file)
        c1_data.options.pch_use = 1;
    if (c1_data.options.pch_auto_file)
        c1_data.options.pch_auto = 1;
    c1_data.options.pch_debug |= c1_data.options.pch_auto;
    if (c1_bss.options.preprocess_stdout || c1_bss.options.prototypes_only) {
        c1_data.options.pch_auto = 0;
        c1_data.options.pch_auto_file = NULL;
        c1_data.options.pch_create = 0;
        c1_data.options.pch_create_file = NULL;
        c1_data.options.pch_use = 0;
        c1_data.options.pch_use_file = NULL;
        c1_data.options.pch_debug = 0;
        c1_data.options.pch_no_preprocess = 0;
    }
    if (c1_data.options.argument_allocation) {
        free_argument_allocation(c1_data.options.argument_allocation);
        c1_data.options.argument_allocation = NULL;
    }
}

void __cdecl initialize_checksums(void) { __asm { int 3 } }
void __cdecl initialize_option_input_12f08(void) { __asm { int 3 } }
void __cdecl initialize_option_input_12f3e(void) { __asm { int 3 } }
void __cdecl read_env_arguments(char *arguments) { __asm { int 3 } }
char *__cdecl duplicate_argument(const char *argument, int copy) { __asm { int 3 } return NULL; }
void __cdecl report_response_error(int code, ...) { __asm { int 3 } }
void __cdecl stop_compiler(int status, int detail) { __asm { int 3 } }
struct CompilerVector *__cdecl new_option_vector(unsigned count, int mode) { __asm { int 3 } return NULL; }
char *__cdecl next_argument(void) { __asm { int 3 } return NULL; }
int __cdecl match_option(CompilerOptionRule *rules, char *argument, char *(__cdecl *next)(void), int copy) { __asm { int 3 } return 0; }
char *__cdecl resolve_option_path(char *output, const char *input, unsigned size) { __asm { int 3 } return NULL; }
void __cdecl disable_multibyte_identifiers(void) { __asm { int 3 } }
void __cdecl initialize_packing(void) { __asm { int 3 } }
int __cdecl compare_pch_paths(const char *left, const char *right) { __asm { int 3 } return 0; }
void __cdecl free_argument_allocation(void *allocation) { __asm { int 3 } }
