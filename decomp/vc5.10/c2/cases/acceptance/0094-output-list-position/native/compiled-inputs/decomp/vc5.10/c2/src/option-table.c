#include "c2.h"
#include <recon.h>

/* The backend's switch records use a selector action, a destination object,
 * and the result returned to its driver. Shared destinations remain aliases. */
int c2_constant_strings;
int c2_stack_probe_limit = 4096;
int c2_data_threshold = 32767;
int c2_integer_size = 4;
int c2_volatile_mode = 3;
int c2_inline_limit, c2_exception_mode;
int c2_loop_limit = 100;
int c2_loop_optimization;
int c2_runtime_ml_debug, c2_runtime_md_debug, c2_runtime_mt_debug;
int c2_runtime_ml, c2_runtime_md, c2_runtime_mt;
int c2_force_inline_float, c2_no_codegen, c2_basic_mode, c2_zero_alignment;
int c2_exception_optimization, c2_no_lock;
int c2_blend = 1;
int c2_p6_mode, c2_fdiv_fix, c2_no_float_inline, c2_profile;
int c2_reproducible, c2_codeview_types, c2_line_numbers, c2_strict_ansi;
int c2_default_library = 1;
int c2_dos_messages;
int c2_windows_calling, c2_entry_hooks, c2_float_calls;
int c2_cpu_generation = 5;
int c2_omit_frame_pointer, c2_calling_cdecl, c2_calling_stdcall, c2_calling_fastcall;
int c2_alias_optimization, c2_global_optimization;
int c2_favor_speed = 1;
int c2_global_optimize, c2_float_precision, c2_frame_optimization;
int c2_warning_level = 1;
int c2_warnings_as_errors;
char *c2_error_file, *c2_source_name, *c2_pdb_name;

extern char *c2_alignment_options;
extern int c2_trace_enabled, c2_internal_error_active;
extern int c2_identifier_limit, c2_assembly_enabled, c2_internal_listing;
extern char *c2_debug_file, *c2_offset_file, *c2_intermediate_base;
extern char *c2_assembly_flags, *c2_assembly_file, *c2_listing_file;
extern char *c2_output_name, *c2_unexpected_argument;

C2Selector c2_machine_selectors[] = {
    { 'w', OPT_ENABLE, &c2_windows_calling },
    { 'h', OPT_ENABLE, &c2_entry_hooks },
    { 'i', OPT_ENABLE, &c2_internal_listing },
    { 'f', OPT_ENABLE, &c2_float_calls },
    { '3', OPT_INTEGER, &c2_cpu_generation },
    { '4', OPT_INTEGER, &c2_cpu_generation },
    { '5', OPT_INTEGER, &c2_cpu_generation },
    { '6', OPT_INTEGER, &c2_cpu_generation },
    { 'y', OPT_ENABLE, &c2_omit_frame_pointer },
    { 'A', OPT_ENABLE, &c2_calling_cdecl },
    { 'D', OPT_ENABLE, &c2_calling_stdcall },
    { 'T', OPT_ENABLE, &c2_calling_fastcall },
    { 'X', OPT_ENABLE, &c2_exception_mode },
    { 0, 0, NULL }
};
C2Selector c2_optimizer_selectors[] = {
    { 'a', OPT_ENABLE, &c2_alias_optimization },
    { 'w', OPT_ENABLE, &c2_global_optimization },
    { 's', OPT_DISABLE, &c2_favor_speed },
    { 't', OPT_ENABLE, &c2_favor_speed },
    { 'g', OPT_ENABLE, &c2_global_optimize },
    { 'p', OPT_ENABLE, &c2_float_precision },
    { 'y', OPT_ENABLE, &c2_frame_optimization },
    { 0, 0, NULL }
};
C2Selector c2_warning_selectors[] = {
    { '0', OPT_INTEGER, &c2_warning_level },
    { '1', OPT_INTEGER, &c2_warning_level },
    { '2', OPT_INTEGER, &c2_warning_level },
    { '3', OPT_INTEGER, &c2_warning_level },
    { '4', OPT_INTEGER, &c2_warning_level },
    { 'X', OPT_ENABLE, &c2_warnings_as_errors },
    { 0, 0, NULL }
};

C2Option c2_options[54] = {
    { "-cbstring", &c2_constant_strings, 1, OPT_ENABLE, 0 },
    { "-Gs#", &c2_stack_probe_limit, 1, OPT_INTEGER, 0 },
    { "-Gt#", &c2_data_threshold, 1, OPT_INTEGER, 0 },
    { "-G#", c2_machine_selectors, 1, OPT_SELECTOR, 0 },
    { "-A#", &c2_alignment_options, 1, OPT_STRING_ONCE, 0 },
    { "-O#", c2_optimizer_selectors, 1, OPT_SELECTOR, 0 },
    { "-W#", c2_warning_selectors, 1, OPT_SELECTOR, 0 },
    { "-db#", &c2_debug_file, 1, OPT_STRING, 0 },
    { "-off#", &c2_offset_file, 1, OPT_STRING, 0 },
    { "-ef#", &c2_error_file, 1, OPT_STRING, 0 },
    { "-f#", &c2_source_name, 1, OPT_STRING, 0 },
    { "-Fd#", &c2_pdb_name, 1, OPT_STRING, 0 },
    { "-QIf", &c2_force_inline_float, 1, OPT_ENABLE, 0 },
    { "-ide", &c2_trace_enabled, 1, OPT_ENABLE, 0 },
    { "-isize#", &c2_integer_size, 1, OPT_INTEGER, 0 },
    { "-il#", &c2_intermediate_base, 1, OPT_STRING, 0 },
    { "-vol#", &c2_volatile_mode, 1, OPT_INTEGER, 0 },
    { "-Inl#", &c2_inline_limit, 1, OPT_INTEGER, 0 },
    { "-EHs", &c2_exception_mode, 1, OPT_ENABLE, 0 },
    { "-EHa", &c2_exception_mode, 1, OPT_DISABLE, 0 },
    { "-Loop#", &c2_loop_limit, 1, OPT_INTEGER, 0 },
    { "-loopopt", &c2_loop_optimization, 1, OPT_ENABLE, 0 },
    { "-MLd", &c2_runtime_ml_debug, 1, OPT_ENABLE, 0 },
    { "-MDd", &c2_runtime_md_debug, 1, OPT_ENABLE, 0 },
    { "-MTd", &c2_runtime_mt_debug, 1, OPT_ENABLE, 0 },
    { "-ML", &c2_runtime_ml, 1, OPT_ENABLE, 0 },
    { "-MD", &c2_runtime_md, 1, OPT_ENABLE, 0 },
    { "-MT", &c2_runtime_mt, 1, OPT_ENABLE, 0 },
    { "-nogen", &c2_no_codegen, 1, OPT_ENABLE, 0 },
    { "-basic", &c2_basic_mode, 1, OPT_ENABLE, 0 },
    { "-bzalign", &c2_zero_alignment, 1, OPT_ENABLE, 0 },
    { "-ehopt", &c2_exception_optimization, 1, OPT_DISABLE, 0 },
    { "-noehopt", &c2_exception_optimization, 1, OPT_ENABLE, 0 },
    { "-nolock", &c2_no_lock, 1, OPT_ENABLE, 0 },
    { "-noblend", &c2_blend, 1, OPT_DISABLE, 0 },
    { "-p6gj", &c2_p6_mode, 1, OPT_ENABLE, 0 },
    { "-QIfdiv", &c2_fdiv_fix, 1, OPT_ENABLE, 0 },
    { "-QI0f", &c2_no_float_inline, 1, OPT_ENABLE, 0 },
    { "-pr", &c2_profile, 1, OPT_ENABLE, 0 },
    { "-FA#", &c2_assembly_flags, 1, OPT_STRING, 0 },
    { "-Fa#", &c2_assembly_file, 1, OPT_STRING, 0 },
    { "-Fs#", &c2_listing_file, 1, OPT_STRING, 0 },
    { "-Bd", &c2_internal_error_active, 1, OPT_ENABLE, 0 },
    { "-Brepro", &c2_reproducible, 1, OPT_ENABLE, 0 },
    { "-Zi", &c2_codeview_types, 1, OPT_ENABLE, 0 },
    { "-Zd", &c2_line_numbers, 1, OPT_ENABLE, 0 },
    { "-Za", &c2_strict_ansi, 1, OPT_ENABLE, 0 },
    { "-Zl", &c2_default_library, 1, OPT_DISABLE, 0 },
    { "-H#", &c2_identifier_limit, 1, OPT_INTEGER, 0 },
    { "-Fo#", &c2_output_name, 1, OPT_STRING, 0 },
    { "-dos", &c2_dos_messages, 1, OPT_ENABLE, 0 },
    { "-S", &c2_assembly_enabled, 1, OPT_ENABLE, 0 },
    { "*", &c2_unexpected_argument, 6, OPT_STRING, 0 },
    { NULL, NULL, 0, 0, 0 }
};
