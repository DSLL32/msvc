#ifndef C1_COMMAND_LINE_H
#define C1_COMMAND_LINE_H
#include <recon.h>

/* Arena-owned pointer vector. Growth/element semantics are separate pending
 * functions; this header is established by its allocation and append code. */
typedef struct CompilerVector {
    short count;
    short capacity;
    int arena_kind;
    void **items;
} CompilerVector;
struct CompilerPacking;
typedef struct CompilerIncludeAlias {
    struct CompilerIncludeAlias *next;
    const char *name;
    const char *replacement;
    int kind, replacement_kind;
} CompilerIncludeAlias;
/* The list root contains one link; node names/layout are recovered separately. */
typedef struct CompilerIncludeAliases {
    struct CompilerIncludeAlias *head;
#ifdef __cplusplus
    const char *RECON_THISCALL Find(const char *, int *);
    void RECON_THISCALL Insert(CompilerIncludeAlias *);
#endif
} CompilerIncludeAliases;

/* These options belong to the initialized compiler-state snapshot. Names
 * follow their switch spelling where the wider option domain is not known. */
typedef struct CompilerOptionData {
    char **argument_allocation;
    struct CompilerPacking *packing_stack;
    struct CompilerIncludeAliases *include_aliases;
    unsigned char warning_levels[1000];
    unsigned lowest_warning;
    unsigned highest_warning;
    int preprocess_only;
    int language_extensions;
    int option_zbe;
    int japanese_identifiers;
    int unsigned_char;
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
    int dependency_output;
    int optimize_size;
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
    int fastcall_calls;
    int incremental_compile;
    int debug_types;
    int function_records;
    int split_pdbs;
    int no_eh_for_new;
    int pch_use;
    char *pch_use_file;
    int pch_create;
    char *pch_create_file;
    int pch_auto;
    char *pch_auto_file;
    char *pch_link_name;
    char *pch_file;
    int pch_no_preprocess;
    char *new_version;
    char *object_file;
    char *database_file;
    int warning_level;
    char *path_separators;
    char *il_prefix;
    int packing_alignment;
    int no_expand_inline;
    char *unrecognized_argument;
    int debug_old_format;
    int move_pch;
} CompilerOptionData;

/* These switches and parser products are genuinely BSS, so they clear on a
 * new pass rather than restoring from the initialized-state snapshot. */
typedef struct CompilerOptionBss {
    int edit_and_continue;
    int option_zn;
    struct CompilerVector *include_directories;
    char *source_file;
    int option_zf;
    char *version_string;
    struct CompilerVector *forced_includes;
    int show_includes;
    struct CompilerVector *defines;
    int prototypes_only;
    char *preprocessed_file;
    char *browser_file; /* RVA54de8, -FR; also enables nested browser references. */
    int preprocess_stdout;
    char *browser_local_file; /* RVA54f2c, -Fr and normalized output filename. */
    int preserve_comments;
    int dump_options;
    int option_xc;
    int warnings_are_errors;
    int suppress_banner;
    int option_zc;
    int fat;
    int pch_memory_limit;
    char *response_file;
    char *error_file;
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

#ifdef __cplusplus
extern "C" {
#endif
extern char c1_default_database[];
extern char c1_default_path_separators[];
extern char c1_empty_string[];
#ifdef __cplusplus
}
#endif

#endif
