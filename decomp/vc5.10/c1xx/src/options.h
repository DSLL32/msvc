#ifndef C1XX_OPTIONS_H
#define C1XX_OPTIONS_H
#include <recon.h>

struct CompilerVector;
struct CompilerPacking;
struct CompilerIncludeAliases;

/* These switches and parser products belong to the initialized snapshot.
 * Widths, defaults and destinations come from C1XX's own option consumers. */
typedef struct CompilerOptionData {
    char ** argument_allocation;
    struct CompilerPacking * packing_stack;
    struct CompilerIncludeAliases * include_aliases;
    unsigned char warning_levels[1000];
    unsigned lowest_warning;
    unsigned highest_warning;
    int preprocess_only;
    int language_extensions;
    int option_zbe;
    int fastcall_calls;
    int japanese_identifiers;
    int unsigned_char;
    int member_pointer_best_case;
    int member_pointer_general;
    int member_pointer_single;
    int member_pointer_multiple;
    int member_pointer_virtual;
    int vtordisp;
    int dll_runtime;
    int multithread_runtime;
    int pascal_calls;
    int debug_info;
    int pch_debug;
    int preprocess_no_lines;
    int option_pf;
    int option_bu;
    int reproducible;
    int intrinsics;
    int stack_checks;
    int cdecl_calls;
    int stdcall_calls;
    int pool_strings;
    int readonly_strings;
    int code_based_strings;
    int processor;
    int minimal_rebuild;
    int inline_level;
    int exception_handling;
    int runtime_type_info;
    int no_bool;
    int dependency_output;
    int incremental_rebuild;
    int optimize_speed;
    int assume_no_alias;
    int assume_no_global_alias;
    int global_optimize;
    int precise_float;
    int omit_frame_pointer;
    int line_records;
    char database_path[256];
    char pch_link_path[256];
    int option_oq;
    int option_ov;
    int incremental_compile;
    int debug_types;
    int function_records;
    int split_pdbs;
    int no_eh_for_new;
    int incremental_rebuild_forced;
    int extern_c_no_throw;
    int pch_use;
    char * pch_use_file;
    int pch_create;
    char * pch_create_file;
    int pch_auto;
    char * pch_auto_file;
    char * pch_link_name;
    char * pch_file;
    int pch_no_preprocess;
    char * new_version;
    char * object_file;
    char * database_file;
    int warning_level;
    char * path_separators;
    char * il_prefix;
    int packing_alignment;
    int no_expand_inline;
    char * unrecognized_argument;
    int debug_old_format;
    int move_pch;
    int option_bcmpconv;
} CompilerOptionData;

/* These distinct destinations clear on each new compiler pass. */
typedef struct CompilerOptionBss {
    int edit_and_continue;
    int option_zn;
    struct CompilerVector * include_directories;
    char * source_file;
    int option_zf;
    char * version_string;
    struct CompilerVector * forced_includes;
    int show_includes;
    struct CompilerVector * defines;
    int prototypes_only;
    char * preprocessed_file;
    char * browser_file;
    int option_binl;
    int preprocess_stdout;
    char * browser_local_file;
    int preserve_comments;
    int dump_options;
    int option_xc;
    int warnings_are_errors;
    int suppress_banner;
    int option_zc;
    int fat;
    int pch_memory_limit;
    char * response_file;
    char * error_file;
} CompilerOptionBss;

typedef struct CompilerOptionRule {
    const char *pattern;
    void *target;
    signed char result;
    unsigned char action;
} CompilerOptionRule;

typedef struct CompilerZRule {
    int letter;
    int action;
    int *target;
} CompilerZRule;

/* The same original empty-string object supplies -il and LC_CTYPE. */
extern const char compiler_empty_string[];
#endif
