#include "compiler-state.h"
#include <recon.h>

/* Original pointer defaults share observable string storage with the option
 * parser and locale call; names preserve those specific identities. */
char c1_default_database[] = "vc50.pdb";
char c1_default_path_separators[] = "\\:/";
char c1_empty_string[] = "";

/* Encountered datafe fields. SYSTEM_INFO starts with the original page and
 * allocation-granularity defaults; GetSystemInfo refreshes it later in main. */
CompilerInitializedState c1_data = {
    1, 0, 0, 0,
    { { 0 }, 4096, NULL, NULL, 0, 0, 0, 65536, 0, 0 },
    0, 0, 100,
    {
        NULL, /* argument_allocation */
        NULL, /* packing_stack */
        NULL, /* include_aliases */
        { 0 }, /* warning_levels */
        1000, /* lowest_warning */
        700, /* highest_warning */
        0, /* preprocess_only */
        0, /* language_extensions */
        0, /* option_zbe */
        0, /* japanese_identifiers */
        0, /* unsigned_char */
        0, /* dll_runtime */
        0, /* multithread_runtime */
        0, /* pascal_calls */
        0, /* debug_info */
        0, /* pch_debug */
        0, /* preprocess_no_lines */
        0, /* option_pf */
        175, /* option_bu */
        0, /* reproducible */
        0, /* intrinsics */
        1, /* stack_checks */
        0, /* cdecl_calls */
        0, /* stdcall_calls */
        0, /* pool_strings */
        0, /* readonly_strings */
        0, /* code_based_strings */
        0, /* processor */
        1, /* minimal_rebuild */
        0, /* inline_level */
        0, /* dependency_output */
        1, /* optimize_size */
        0, /* assume_no_alias */
        0, /* assume_no_global_alias */
        0, /* global_optimize */
        0, /* precise_float */
        0, /* omit_frame_pointer */
        1, /* line_records */
        { 0 }, /* database_path */
        { 0 }, /* pch_link_path */
        0, /* option_oq */
        0, /* option_ov */
        0, /* fastcall_calls */
        0, /* incremental_compile */
        0, /* debug_types */
        1, /* function_records */
        1, /* split_pdbs */
        0, /* no_eh_for_new */
        0, /* pch_use */
        NULL, /* pch_use_file */
        0, /* pch_create */
        NULL, /* pch_create_file */
        0, /* pch_auto */
        NULL, /* pch_auto_file */
        NULL, /* pch_link_name */
        NULL, /* pch_file */
        1, /* pch_no_preprocess */
        NULL, /* new_version */
        NULL, /* object_file */
        c1_default_database, /* database_file */
        1, /* warning_level */
        c1_default_path_separators, /* path_separators */
        c1_empty_string, /* il_prefix */
        -1, /* packing_alignment */
        0, /* no_expand_inline */
        NULL, /* unrecognized_argument */
        0, /* debug_old_format */
        0, /* move_pch */
    },
    C1_SCANNER_DEFAULTS,
    NULL, NULL, 0, 0, 0, NULL, 0,
    { 0 }, 0, 0, 0, 0, 1, 0, 0, NULL, NULL, 0, { 0 }, NULL, NULL, 3, 1, 1, 0, 0, NULL, NULL, NULL, { 0 }, 9, NULL, NULL, 3, 9, 16, 9, NULL, NULL, "......Unknown date......", 0, -1, 0, NULL, NULL, NULL, -1, -1,
    { { { 0 }, 52, { 0 }, 52, { 0 }, 52, { 0 } } }
};
CompilerBss c1_bss;
int c1_reuse_state;
CompilerPdbContext *c1_current_pdb_context;

/* All34 actual .initfe slots are represented; unrecovered consumers trap. */
C1Initializer const c1_initialization_callbacks[34] = {
    NULL,
    initialize_state_12365,
    initialize_state_12377,
    initialize_state_12389,
    initialize_state_123a4,
    initialize_state_123a5,
    initialize_state_123a6,
    initialize_state_123a7,
    initialize_state_1241c,
    initialize_state_1246b,
    initialize_state_124ba,
    initialize_state_12509,
    initialize_state_12558,
    initialize_state_125a7,
    initialize_state_125f6,
    initialize_state_12645,
    initialize_state_12646,
    initialize_state_12663,
    initialize_state_12664,
    initialize_state_1266f,
    initialize_state_126af,
    initialize_state_126ef,
    initialize_state_12712,
    initialize_state_1271f,
    initialize_state_1272c,
    initialize_state_12739,
    initialize_state_12746,
    initialize_state_12753,
    initialize_state_12760,
    initialize_state_1276d,
    initialize_state_1277a,
    initialize_state_1277b,
    initialize_state_1279c,
    initialize_state_127a6
};
