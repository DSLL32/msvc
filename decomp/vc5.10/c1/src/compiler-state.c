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
    { 0 }, 0, 0, 0, 0, 1, 0, 0, NULL, NULL, 0, { 0 }, NULL, NULL, 3, 1, 1, 0, 0, NULL, NULL, NULL, { 0 }, 9, NULL, 3, 9, 16, 9, NULL, NULL, "......Unknown date......", 0, -1, 0, NULL, NULL, NULL, -1, -1, NULL, 2, 0, 9, NULL, 0x3fffff, 1, 0, 0, NULL, 0, -1, -1, 0, 1, NULL, 0, 0, 0,
    NULL, NULL, 19, 0, 0, 0, NULL, 0, 0, 0, /* Function directory and function/debug/source state. */
    0, NULL, NULL, /* PCH type value, record name and serialized options. */
    0, 0, 0, 0, /* PCH stream-open flags and cleanup marker. */
    NULL, 0, /* Empty saved-context table. */
    -1, NULL, -1, /* Tag diagnostic mode, active enum type and last value. */
    10, 5, /* Actual tag/enumerator symbol cache indexes. */
    c1_bss.quoted_workspace, /* Actual initialized quoted-spelling cursor. */
    9, NULL, /* Parameter-symbol cache selector and lazy sentinel identity. */
    0, /* PCH creation filename has not been encountered. */
    0, /* Data-linkage option5ef9c is disabled. */
    0, 0, 0, /* Writer-only initializer reset cells. */
    -1, NULL, /* Initializer frame depth and last published symbol. */
    0, /* Unevaluated type nesting. */
    9, /* Actual kind7 symbol-cache selector. */
    9, /* Actual kind10 symbol-cache selector. */
    { /* Message IDs and initial NULL borrowed-text slots, through the zero sentinel. */
        { C1_TEXT_UNNAMED_TOKEN, NULL },
        { C1_TEXT_UNNAMED_LABEL, NULL },
        { C1_TEXT_OLD_VARIADIC_COMMA, NULL },
        { C1_TEXT_BAD_NUMBER_SUFFIX, NULL },
        { C1_TEXT_UNKNOWN_WORD, NULL },
        { C1_TEXT_UNKNOWN_LABEL, NULL },
        { C1_TEXT_NOTE, NULL },
        { C1_TEXT_WARNING, NULL },
        { C1_TEXT_ERROR, NULL },
        { C1_TEXT_FATAL_ERROR, NULL },
        { C1_TEXT_FIRST_OPERAND, NULL },
        { C1_TEXT_SECOND_OPERAND, NULL },
        { C1_TEXT_THIRD_OPERAND, NULL },
        { C1_TEXT_OPCODE, NULL },
        { C1_TEXT_ANONYMOUS_UNION, NULL },
        { C1_TEXT_MEMBER, NULL },
        { C1_TEXT_ENUMERATOR, NULL },
        { C1_TEXT_INACCESSIBLE, NULL },
        { C1_TEXT_INPUT, NULL },
        { C1_TEXT_NON_STATIC, NULL },
        { C1_TEXT_COMPILER_INTERMEDIATE, NULL },
        { C1_TEXT_COMPILER_GENERATED, NULL },
        { C1_TEXT_SOURCE, NULL },
        { C1_TEXT_INLINE_FUNCTION_DEFINITION, NULL },
        { C1_TEXT_INCLUDE, NULL },
        { C1_TEXT_PRECOMPILED_HEADER, NULL },
        { C1_TEXT_OVERLOADED_FUNCTION_TYPE, NULL },
        { C1_TEXT_OPERATOR_NEW_TARGET, NULL },
        { C1_TEXT_SINGLE_INHERITANCE, NULL },
        { C1_TEXT_MULTIPLE_INHERITANCE, NULL },
        { C1_TEXT_VIRTUAL_INHERITANCE, NULL },
        { C1_TEXT_USING_PRECOMPILED_HEADER, NULL },
        { C1_TEXT_CREATING_PRECOMPILED_HEADER, NULL },
        { C1_TEXT_CREATING_NEW_PRECOMPILED_HEADER, NULL },
        { C1_TEXT_INCOMPATIBLE_OPTIONS, NULL },
        { C1_TEXT_PROGRAM_DATABASE, NULL },
        { C1_TEXT_BAD_PATH, NULL },
        { C1_TEXT_DEBUG, NULL },
        { C1_TEXT_BROWSER, NULL },
        { C1_TEXT_TYPE_LIBRARY, NULL },
        { C1_TEXT_BRACE, NULL },
        { C1_TEXT_PARENTHESIS, NULL },
        { C1_TEXT_BRACKET, NULL },
        { C1_TEXT_BLOCK_COMMENT, NULL },
        { C1_TEXT_ANGLE_BRACKET, NULL },
        { C1_TEXT_DEFAULT_CONSTRUCTOR, NULL },
        { C1_TEXT_COPY_CONSTRUCTOR, NULL },
        { C1_TEXT_ASSIGNMENT_OPERATOR, NULL },
        { C1_TEXT_DESTRUCTOR, NULL },
        { C1_TEXT_FUNCTION, NULL },
        { C1_TEXT_EMPTY_DECLARATION, NULL },
        { C1_TEXT_STATIC_DATA_MEMBER, NULL },
        { C1_TEXT_DATA, NULL },
        { C1_TEXT_FUNCTION_THROUGH_POINTER, NULL },
        { C1_TEXT_ARGUMENT, NULL },
        { C1_TEXT_COULD_BE, NULL },
        { C1_TEXT_OR, NULL },
        { C1_TEXT_MEMORY_MODEL_OPTION, NULL },
        { C1_TEXT_DATABASE_NAME_OPTION, NULL },
        { C1_TEXT_NATIVE_CALLER_OPTION, NULL },
        { C1_TEXT_PASCAL_CALL_OPTION, NULL },
        { C1_TEXT_FASTCALL_OPTION, NULL },
        { C1_TEXT_UNSIGNED_CHAR_OPTION, NULL },
        { C1_TEXT_MEMBER_POINTER_BEST_CASE_OPTION, NULL },
        { C1_TEXT_MEMBER_POINTER_GENERAL_OPTION, NULL },
        { C1_TEXT_MEMBER_POINTER_INHERITANCE_OPTION, NULL },
        { C1_TEXT_VTORDISP_OPTION, NULL },
        { C1_TEXT_DLL_RUNTIME_OPTION, NULL },
        { C1_TEXT_BITFIELD_LAYOUT_OPTION, NULL },
        { C1_TEXT_OPTIMIZATION_OPTIONS, NULL },
        { C1_TEXT_EXCEPTION_UNWINDING_OPTION, NULL },
        { C1_TEXT_BROWSER_OPTION, NULL },
        { C1_TEXT_LIBRARY_PCT_OPTION, NULL },
        { C1_TEXT_PACKING_OPTION, NULL },
        { C1_TEXT_RTTI_OPTION, NULL },
        { C1_TEXT_INCLUDED_FILE_NOTE, NULL },
        { C1_TEXT_BOOL_OPTION, NULL },
        { C1_TEXT_INCLUDE_TWO_TYPES_NOTE, NULL },
        { C1_TEXT_INCLUDE_TYPE_NOTE, NULL },
        { C1_TEXT_NEW_PCH_NOTE, NULL },
        { C1_TEXT_OPTIONS_CHANGED_NOTE, NULL },
        { C1_TEXT_UPDATE_PCH_MEMORY_NOTE, NULL },
        { C1_TEXT_UPDATE_PCH_MEMORY_FAILED, NULL },
        { C1_TEXT_PCH_TIMESTAMP_FAILED, NULL },
        { C1_TEXT_REUSE_PCH_NOTE, NULL },
        { C1_TEXT_MEMORY_RELEASE_NOTE, NULL },
        { 0, NULL }
    },
    0, /* Lazy compiler message initialization, RVA5f8a0. */
    9, /* Actual kind3 member-symbol cache selector. */
    9, /* Actual kind14 segment-symbol cache selector. */
    NULL, /* Record-id declaration-context map. */
    1, /* Generated anonymous-identifier sequence. */
    8, 0, /* Inline depth and recursion pragma state. */
    0x03fff800, 0x037eb800, /* Empty optimize-string restore/disable masks. */
    { { { 0 }, 52, 52, 52, 52, 40, 52, { 0 }, 52, 52, 52, 52, 52, 52, { 0 } } },
    { { { 0 }, NULL, { 0 }, NULL, NULL } }, /* Only heads5/9/10 have known defaults. */
    { NULL, NULL, NULL }, /* BSS/CONST/DATA segment defaults are semantic NULL. */
    NULL, /* void_type_list */
    NULL, /* call_context */
    1, 0, /* Conditional-expression lookahead and parenthesis depth. */
    0, 100, /* Diagnostic type-text context depth and next handle. */
    (struct CompilerParserToken *)&c1_bss.assembly_empty_token, 0, 0,
    0, 0, NULL, /* Diagnostic prefix length, line and source. */
    0, 0, NULL, /* Browser last line, absolute mode and current output stream. */
    0, 0, 0, /* Browser suppression, header position and generation. */
    0, /* Browser scope depth. */
    0, 0, 0 /* Declaration-format failure marker, function flag and spelling flags. */

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
