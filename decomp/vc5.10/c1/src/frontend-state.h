#ifndef C1_FRONTEND_STATE_H
#define C1_FRONTEND_STATE_H
#include <recon.h>

struct CompilerBackend;
struct CompilerTranslationUnit;
struct CompilerDependency;
struct CompilerInput;
struct CompilerDebugModule;

/* Known settings within the original 1028-byte PCH option snapshot. These
 * fields are a typed projection; 340 original bytes remain UNMODELED. */
typedef struct CompilerPchSettings {
    int preprocess_only, language_extensions, option_zbe, internal_zb_value;
    int fastcall_calls, japanese_identifiers, unsigned_char;
    int dll_runtime, multithread_runtime, pascal_calls, debug_info, pch_debug;
    int preprocess_no_lines, option_pf, option_bu, reproducible;
    int intrinsics, stack_checks, cdecl_calls, stdcall_calls;
    int pool_strings, readonly_strings, code_based_strings, processor;
    int minimal_rebuild, inline_level;
    unsigned option_flags;
    int dependency_output, dependency_local;
    int optimize_size, assume_no_alias, assume_no_global_alias, global_optimize;
    int precise_float, omit_frame_pointer, line_records;
    char database_path[256], pch_link_path[256];
    int option_oq, option_ov, c9_il, incremental_compile, debug_types;
    int function_records, split_pdbs, no_eh_for_new;
} CompilerPchSettings;
#endif
