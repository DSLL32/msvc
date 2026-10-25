#ifndef C1_FRONTEND_STATE_H
#define C1_FRONTEND_STATE_H
#include "c1.h"
#include <recon.h>

struct CompilerBackend;
struct CompilerTranslationUnit;
struct CompilerDependency;
struct CompilerInput;
struct CompilerDebugModule;
struct CompilerTextInput;
struct CompilerLexer;
struct CompilerTypeShape;
struct CompilerNameEntry;

typedef struct CompilerType {
    WORD flags, attributes;
    struct CompilerTypeShape *shape;
    unsigned auxiliary;
    /* Type interning compares only the first twelve bytes. Debug builds
     * initialize this cache separately; the scratch descriptor leaves it. */
    unsigned debug_index;
} CompilerType;
typedef char CompilerType_size[sizeof(CompilerType) == 16 ? 1 : -1];

typedef struct CompilerBuiltinTypes {
    CompilerType *type_0000, *type_4041_first, *type_0063, *type_4063_first;
    CompilerType *type_0041, *type_4041_second, *type_0042, *type_4042_first;
    CompilerType *type_0043, *type_4043, *type_004c, *type_404c, *type_0040;
    CompilerType *type_4063_second, *type_0042_second, *type_4042_second;
    CompilerType *type_0044, *type_4044, *type_0106, *type_0107, *type_0108;
    CompilerType *type_0802, *type_0080;
    CompilerType *type_0041_q0100, *type_4041_q0100, *type_0063_q0100;
    CompilerType *implicit_int_function, *tagged_0080;
} CompilerBuiltinTypes;

typedef struct CompilerLexerBackend {
    int enabled;
    /* A later declaration callback sets this before consuming it. */
    int attribute_mode;
    struct CompilerBackend *backend;
} CompilerLexerBackend;

/* Known settings within the original 1028-byte PCH option snapshot. These
 * fields are a typed projection; 316 original bytes remain UNMODELED. */
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
    int normalized_pool_strings, normalized_readonly_strings;
    int browser_enabled, browser_full, normalized_stack_checks, normalized_oq;
} CompilerPchSettings;
#endif
