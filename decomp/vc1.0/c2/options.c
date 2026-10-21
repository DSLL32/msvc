/* Backend switches and their storage, introduced with parse_options. */
#include "options.h"
#include "functions.h"
#include <string.h>
char *source_filename, *assembly_filename, *debug_filename, *listing_filename;
char *object_filename, *unknown_option, *assembly_options, *error_filename, *language_options;
int assembly_output, assembly_code, listing_output, assembly_source, object_name_explicit;
int assembly_cleanup_pending; /* 0008315c */
int byte_block_second, byte_block_first, identifier_limit = 247;
char *generated_source = "compiler generated";

/* Original [00026a10,00026b60). */
void parse_options(void)
{
    char *previous = debug_filename;
    int result;
    for (;;) {
        result = match_option(compiler_options, next_argument(next_argument, 0), (char *(__cdecl *)(void))next_argument, 0);
        if (!result) break;
        if (result == 6) compiler_report(7, unknown_option, "p2");
        if (previous != debug_filename) previous = debug_filename;
    }
    consume_string(language_options);
    if (byte_block_second) byte_block_first = 1;
    if (object_filename) object_name_explicit = 1;
    if (identifier_limit > 247) identifier_limit = 247;
    if (assembly_options) {
        assembly_output = 1;
        if (strpbrk(assembly_options, "cC")) assembly_code = 1;
        if (strpbrk(assembly_options, "sS")) assembly_source = 1;
    }
    if (listing_filename) listing_output = 1;
    if (assembly_filename) {
        assembly_output = 1;
        if (!freopen(assembly_filename, "wt", stdout))
            report_context_error(0x53, generated_source, assembly_filename);
        assembly_cleanup_pending = 1;

    }
}

/* Scalar destinations are named after the switches that introduce them. */
int string_blocks;
int stack_probe_size = 4096, data_threshold = 32767;
int optimization_align = 1, optimization_combine = 1, optimization_cost;
int optimization_double = 1, optimization_float_range = 1, optimization_float_registers = 1;
int optimization_float_peephole = 1, optimization_float_order = 1;
int optimization_immediate_registers = 1, optimization_jumps = 1;
int optimization_jump_schedule = 1, optimization_range = 1, force_risc;
int optimization_recycle = 1, optimization_risc = 1, optimization_schedule = 1;
int optimization_schedule_model, optimization_split = 1, optimization_stack = 1;
int optimization_unroll = 1, optimize_global, optimize_alias, optimize_weak;
int optimize_speed = 1, optimize_precision, omit_frame_pointer;
int optimize_stack_checks, optimize_stack_check_inverse;
int windows_prolog, hook_functions, floating_prolog, cpu_model = 3, frame_pointer;
int runtime_single, runtime_dll, runtime_multi;
int warning_level = 1, warnings_as_errors;
int byte_align, print_registers, debug_commands, reproducible_build;
int codeview_debug, line_debug, ansi_mode, default_library = 1;
int integer_size = 4, dos_object, generate_disabled;
int coff_object = 1, seh_disabled, c6_compatibility;
Suboption optimize_options[] = {
    {'a', 1, &optimize_alias}, {'w', 1, &optimize_weak},
    {'s', 5, &optimize_speed}, {'t', 1, &optimize_speed},
    {'g', 1, &optimize_global}, {'p', 1, &optimize_precision},
    {'y', 1, &omit_frame_pointer}, {0, 0, NULL}
};
Suboption generation_options[] = {
    {'w', 1, &windows_prolog}, {'h', 1, &hook_functions},
    {'f', 1, &floating_prolog}, {'3', 36, &cpu_model},
    {'4', 36, &cpu_model}, {'5', 36, &cpu_model},
    {'y', 1, &frame_pointer}, {0, 0, NULL}
};
Suboption warning_options[] = {
    {'0', 36, &warning_level}, {'1', 36, &warning_level},
    {'2', 36, &warning_level}, {'3', 36, &warning_level},
    {'4', 36, &warning_level}, {'X', 1, &warnings_as_errors},
    {0, 0, NULL}
};
#define SWITCH(pattern, variable, action) {pattern, &variable, 1, action, 0}
#define SUBOPTIONS(pattern, table) {pattern, table, 1, 35, 0}
Option compiler_options[] = {
    SWITCH("-cbstring", string_blocks, 1),
    SWITCH("-Gs#", stack_probe_size, 36),
    SWITCH("-Gt#", data_threshold, 36),
    SUBOPTIONS("-G#", generation_options),
    SWITCH("-A#", language_options, 39),
    SUBOPTIONS("-O#", optimize_options),
    SUBOPTIONS("-W#", warning_options),
    SWITCH("-db#", debug_filename, 34),
    SWITCH("-ef#", error_filename, 34),
    SWITCH("-f#", source_filename, 34),
    SWITCH("-ide", debugger_enabled, 1),
    SWITCH("-isize#", integer_size, 36),
    SWITCH("-il#", intermediate_prefix, 34),
    SWITCH("-ML", runtime_single, 1),
    SWITCH("-MD", runtime_dll, 1),
    SWITCH("-MT", runtime_multi, 1),
    SWITCH("-nogen", generate_disabled, 1),
    SWITCH("-bzalign", byte_align, 1),
    SWITCH("-noalign", optimization_align, 5),
    SWITCH("-nocombine", optimization_combine, 5),
    SWITCH("-nodouble", optimization_double, 5),
    SWITCH("-nofrange", optimization_float_range, 5),
    SWITCH("-nofreg", optimization_float_registers, 5),
    SWITCH("-nofpeep", optimization_float_peephole, 5),
    SWITCH("-noforder", optimization_float_order, 5),
    SWITCH("-noimmreg", optimization_immediate_registers, 5),
    SWITCH("-nojmp", optimization_jumps, 5),
    SWITCH("-nojsched", optimization_jump_schedule, 5),
    SWITCH("-norange", optimization_range, 5),
    SWITCH("-forcerisc", force_risc, 1),
    SWITCH("-norecy", optimization_recycle, 5),
    SWITCH("-norisc", optimization_risc, 5),
    SWITCH("-nosched", optimization_schedule, 5),
    SWITCH("-nospl", optimization_split, 5),
    SWITCH("-nostack", optimization_stack, 5),
    SWITCH("-nounroll", optimization_unroll, 5),
    SWITCH("-sched#", optimization_schedule_model, 36),
    SWITCH("-cost#", optimization_cost, 36),
    SWITCH("-pr", print_registers, 1),
    SWITCH("-FA#", assembly_options, 34),
    SWITCH("-Fa#", assembly_filename, 34),
    SWITCH("-Fs#", listing_filename, 34),
    SWITCH("-Bd", debug_commands, 1),
    SWITCH("-Brepro", reproducible_build, 1),
    SWITCH("-Zi", codeview_debug, 1),
    SWITCH("-Zd", line_debug, 1),
    SWITCH("-Za", ansi_mode, 1),
    SWITCH("-Zl", default_library, 5),
    SWITCH("-H#", identifier_limit, 36),
    SWITCH("-Fo#", object_filename, 34),
    SWITCH("-dos", dos_object, 1),
    SWITCH("-S", assembly_output, 1),
    SWITCH("-Bbb2", byte_block_second, 1),
    SWITCH("-Bbb1", byte_block_first, 1),
    SWITCH("-omf", coff_object, 5),
    SWITCH("-noxseh", seh_disabled, 1),
    SWITCH("-c6386", c6_compatibility, 1),
    {"*", &unknown_option, 6, 34, 0},
    {NULL, NULL, 0, 0, 0}
};

void __fastcall apply_option(Option *option, char *value, int copy);
int __fastcall suffix_matches(char *pattern, char *text, unsigned action);
char *current_option, *last_option_value;
/* Original [00070c90,00070dc0). */
int __fastcall match_option(Option *options, char *argument, char *(__cdecl *next)(void), int copy)
{
    Option *option;
    char *pattern, *value;
    if (!argument) return 0;
    current_option = argument;
    for (option = options; option->action; ++option) {
        pattern = option->pattern;
        value = argument;
        for (;;) {
            if (*pattern == '#') {
                if (!*value) last_option_value = value = next();
                apply_option(option, value, copy);
                return option->result;
            }
            if (*pattern == '*') {
                if (*value && suffix_matches(pattern, value, option->action)) {
                    apply_option(option, value, copy);
                    return option->result;
                }
                break;
            }
            if (!*pattern) {
                if (*value) break;
                if (option->action & 0x20) last_option_value = value = next();
                else value = NULL;
                apply_option(option, value, copy);
                return option->result;
            }
            if (*value++ != *pattern++) break;
        }
    }
    return 0;
}

long __fastcall parse_number(char *text);
/* Original [00070dc0,00070fb0). */
void __fastcall apply_option(Option *option, char *value, int copy)
{
    int *integer;
    int count;
    char *pattern, *end;
    Suboption *suboptions, *suboption;
    switch (option->action) {
    case 1: *(int *)option->destination = 1; return;
    case 5: *(int *)option->destination = 0; return;
    case 36: *(int *)option->destination = parse_number(value); return;
    case 38:
        integer = (int *)option->destination;
        count = *integer;
        if (count > 0) {
            *integer = --count;
            ((char **)integer)[count + 1] = copy ? _strdup(value) : value;
        } else compiler_report(0x4a, option->pattern, value);
        return;
    case 39:
        if (*(char **)option->destination) {
            pattern = _strdup(option->pattern);
            end = pattern;
            while (*end) ++end;
            end[-1] = 0;
            compiler_report(0x49, pattern, *(char **)option->destination, value);
            return;
        }
        /* A first occurrence stores the string using action 34. */
    case 34:
        *(char **)option->destination = copy ? _strdup(value) : value;
        return;
    case 35:
        suboptions = (Suboption *)option->destination;
        while (*value) {
            suboption = suboptions;
            if (suboption->letter) {
                while ((char)suboption->letter != *value) {
                    ++suboption;
                    if (!suboption->letter) break;
                }
            }
            if (suboption->letter) {
                switch (suboption->action) {
                case 1: *suboption->destination = 1; break;
                case 5: *suboption->destination = 0; break;
                case 36: *suboption->destination = *value - '0'; break;
                default: compiler_internal_error("@(#)getflags.c:1.30", 0xd1); break;
                }
            } else compiler_report(0x30, (int)*value, current_option);
            ++value;
        }
        return;
    default: compiler_internal_error("@(#)getflags.c:1.30", 0xdb); return;
    }
}

#include <ctype.h>
#include <errno.h>
/* Original [00071010,000710b0). */
long __fastcall parse_number(char *text)
{
    char *start = text;
    char *end;
    long result;
    while (isspace(*start)) ++start;
    errno = 0;
    result = strtol(start, &end, 0);
    if (errno == ERANGE || *end) compiler_report(0x31, text);
    return result;
}

/* Original [00026b60,00026b70). */
void __fastcall consume_string(char *string)
{
    if (string) while (*string) ++string;
}

int function_optimization;      /* 000806ac */
int global_optimization_active; /* 00080680 */
int optimize_weak_inverse;      /* 00080690 */
/* Original [00026b70,00026c50). Restore the frontend's per-function options. */
void __fastcall apply_function_options(unsigned bits)
{
    function_optimization = (bits & 0x80000000U) != 0;
    optimize_alias = (bits & 0x02000000) != 0;
    optimize_weak = (bits & 0x01000000) != 0;
    optimize_speed = (bits & 0x00800000) != 0;
    optimize_global = (bits & 0x00200000) != 0;
    optimize_precision = (bits & 0x00040000) != 0;
    omit_frame_pointer = (bits & 0x00000800) != 0;
    if (optimize_global) global_optimization_active = 1;
    optimize_weak_inverse = !optimize_weak;
    if (optimize_weak) optimize_alias = 1;
    if (seh_disabled && (current_symbol->symbol.field0e & 0x1000)) {
        global_optimization_active = 0;
        optimize_global = 0;
    }
}
