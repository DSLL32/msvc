#ifndef C1XX_FRONTEND_DRIVER_H
#define C1XX_FRONTEND_DRIVER_H
#include <recon.h>

struct CompilerBackend;
struct CompilerTranslationUnit;
struct CompilerDependency;
struct CompilerInput;
struct CompilerDebugModule;

/* Known fields in original cd090..cd4ac, copied to c4f68..c5384.
 * This is a typed projection of the 1052-byte PCH settings copy, not a
 * byte image. Remaining storage has no native placeholder or default claim. */
typedef struct CompilerPchSettings {
    int preprocess_only; /* RVA cd090 */
    int language_extensions; /* RVA cd094 */
    int option_zbe; /* RVA cd098 */
    int option_zb; /* RVA cd09c */
    int fastcall_calls; /* RVA cd0a0 */
    int japanese_identifiers; /* RVA cd0a4 */
    int unsigned_char; /* RVA cd0a8 */
    int member_pointer_best_case; /* RVA cd0ac */
    int member_pointer_general; /* RVA cd0b0 */
    int member_pointer_single; /* RVA cd0b4 */
    int member_pointer_multiple; /* RVA cd0b8 */
    int member_pointer_virtual; /* RVA cd0bc */
    int vtordisp; /* RVA cd0c0 */
    int dll_runtime; /* RVA cd0c4 */
    int multithread_runtime; /* RVA cd0c8 */
    int pascal_calls; /* RVA cd0cc */
    int debug_info; /* RVA cd0d0 */
    int pch_debug; /* RVA cd0d4 */
    int preprocess_no_lines; /* RVA cd0dc */
    int option_pf; /* RVA cd0e0 */
    int option_bu; /* RVA cd0e4 */
    int reproducible; /* RVA cd0e8 */
    int intrinsics; /* RVA cd0f0 */
    int stack_checks; /* RVA cd0f4 */
    int cdecl_calls; /* RVA cd100 */
    int stdcall_calls; /* RVA cd104 */
    int pool_strings; /* RVA cd108 */
    int readonly_strings; /* RVA cd110 */
    int code_based_strings; /* RVA cd118 */
    int processor; /* RVA cd11c */
    int minimal_rebuild; /* RVA cd120 */
    int inline_level; /* RVA cd124 */
    int exception_handling; /* RVA cd138 */
    int runtime_type_info; /* RVA cd13c */
    int no_bool; /* RVA cd140 */
    int dependency_output; /* RVA cd148 */
    int incremental_rebuild; /* RVA cd14c */
    int optimize_speed; /* RVA cd154 */
    int assume_no_alias; /* RVA cd158 */
    int assume_no_global_alias; /* RVA cd15c */
    int global_optimize; /* RVA cd160 */
    int precise_float; /* RVA cd164 */
    int omit_frame_pointer; /* RVA cd168 */
    int line_records; /* RVA cd16c */
    char database_path[256]; /* RVA cd170 */
    char pch_link_path[256]; /* RVA cd270 */
    int option_oq; /* RVA cd474 */
    int option_ov; /* RVA cd47c */
    int option_c9il; /* RVA cd488 */
    int incremental_compile; /* RVA cd48c */
    int debug_types; /* RVA cd490 */
    int function_records; /* RVA cd498 */
    int split_pdbs; /* RVA cd49c */
    int no_eh_for_new; /* RVA cd4a0 */
    int incremental_rebuild_forced; /* RVA cd4a4 */
    int extern_c_no_throw; /* RVA cd4a8 */
    unsigned pch_option_flags; /* RVA cd130 */
    int normalized_stack_checks, normalized_oq;
    int normalized_pool_strings, normalized_readonly_strings;
    int check_completed_class_option; /* RVA cd144 -> c501c in PCH copy */
    int browser_enabled, browser_full;
} CompilerPchSettings;
#endif
