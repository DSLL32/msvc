/* Frontend argument setup, original 0x3cfa0..0x3d216. */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "options.h"

unsigned char warning_levels[800];
unsigned lowest_warning_700 = 800, highest_warning_700 = 700;
char database_path[256], pch_object_name[256];
char **argument_vector_allocation;
PackingStack *packing_stack;
int packing_alignment = -1;
CompilerOptions options = { 0, 0, 0, 1, "msvc.pdb" };
/* The parser receives this table only after its own named frontier. */
OptionRule *option_rules;

typedef struct WarningDefault {
    unsigned code;
    unsigned char flags;
    unsigned char reserved[3];
} WarningDefault;

static WarningDefault warning_defaults[] = {
    { 1, 5, {0,0,0} },
    { 49, 5, {0,0,0} },
    { 69, 5, {0,0,0} },
    { 611, 5, {0,0,0} },
    { 226, 14, {0,0,0} },
    { 0, 0, {0,0,0} }
};

void parse_options(int argc, char **argv)
{
    char resolved[520];
    unsigned length;
    WarningDefault *warning;

    push_scope(0);
    if (!packing_stack)
        initialize_packing();
    initialize_checksums();
    initialize_lead_bytes();
    read_env_arguments(getenv("MSC_CMD_FLAGS"));
    for (warning = warning_defaults; warning->code; ++warning) {
        warning_levels[warning->code] = warning->flags;
        if (warning->code >= 700 && warning->code < 800) {
            if (lowest_warning_700 >= warning->code)
                lowest_warning_700 = warning->code;
            if (highest_warning_700 <= warning->code)
                highest_warning_700 = warning->code;
        }
    }
    options.defines = new_vector(40, -1);
    options.include_directories = new_vector(40, -1);
    options.forced_includes = new_vector(40, -1);
    while (match_option(option_rules, next_argument(), next_argument, 0))
        ;
    if (options.japanese_identifiers)
        initialize_japanese_characters();
    if (options.debug_old_format) {
        options.debug_info = 1;
        options.line_records = 0;
    }
    options.line_records &= options.debug_info;
    if (!_fullpath(resolved, options.database_file, 520))
        fatal_error(83, "database", options.database_file, "bad path");
    for (length = 0; resolved[length]; ++length)
        ;
    if (length >= 256)
        fatal_error(5);
    for (length = 0; (database_path[length] = resolved[length]) != 0; ++length)
        ;
    if (options.pch_link_name) {
        for (length = 0; options.pch_link_name[length]; ++length)
            ;
        if (length >= 256)
            fatal_error(5);
        for (length = 0; (pch_object_name[length] = options.pch_link_name[length]) != 0; ++length)
            ;
    }
    if (options.unrecognized_argument)
        fatal_error(7, options.unrecognized_argument, "c1");
    if (!options.source_file)
        fatal_error(8);
    if (options.syntax_only) {
        options.pch_auto = 0;
        options.pch_auto_file = 0;
        options.pch_create = 0;
        options.pch_create_file = 0;
        options.pch_use = 0;
        options.pch_use_file = 0;
    }
    finalize_options();
    if (argument_vector_allocation) {
        free(argument_vector_allocation);
        argument_vector_allocation = 0;
    }
}
