/* Frontend argument setup, original 0x3cfa0..0x3d216. */
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "options.h"

unsigned char warning_levels[800];
unsigned lowest_warning_700 = 800, highest_warning_700 = 700;
char **argument_vector_allocation;
PackingStack *packing_stack;

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

    push_linkage(LINKAGE_DEFAULT);
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
    if (options.configuration.japanese_identifiers)
        initialize_japanese_characters();
    if (options.debug_old_format) {
        options.configuration.debug_info = 1;
        options.configuration.line_records = 0;
    }
    options.configuration.line_records &= options.configuration.debug_info;
    if (!_fullpath(resolved, options.database_file, 520))
        fatal_error(83, "database", options.database_file, "bad path");
    for (length = 0; resolved[length]; ++length)
        ;
    if (length >= 256)
        fatal_error(5);
    for (length = 0; (options.configuration.database_path[length] = resolved[length]) != 0; ++length)
        ;
    if (options.pch_link_name) {
        for (length = 0; options.pch_link_name[length]; ++length)
            ;
        if (length >= 256)
            fatal_error(5);
        for (length = 0; (options.configuration.pch_object_name[length] = options.pch_link_name[length]) != 0; ++length)
            ;
    }
    if (options.unrecognized_argument)
        fatal_error(7, options.unrecognized_argument, "c1");
    if (!options.source_file)
        fatal_error(8);
    if (options.prototypes_only) {
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

char *current_option_argument; /* original 0x9a15c */

/* Original 0x73840..0x7395d. */
int match_option(OptionRule *rules, char *argument, char *(*next)(void), int copy)
{
    OptionRule *rule;
    char *pattern, *value;
    int c;
    if (!argument)
        return 0;
    current_option_argument = argument;
    for (rule = rules; rule->action; ++rule) {
        pattern = rule->pattern;
        value = argument;
        for (;;) {
            c = *pattern;
            if (!c) {
                if (*value)
                    break;
                apply_option(rule, (rule->action & 0x20) ? next() : 0, copy);
                return rule->result;
            }
            if (c == '#') {
                apply_option(rule, *value ? value : next(), copy);
                return rule->result;
            }
            if (c == '*') {
                if (!*value || !match_option_suffix(pattern, value, rule->action))
                    break;
                apply_option(rule, value, copy);
                return rule->result;
            }
            if (c == '-') {
                if (!prefix_character("-/", *value))
                    break;
                ++value;
                ++pattern;
            } else if (*value++ != *pattern++)
                break;
        }
    }
    return 0;
}


/* Original 0x73b80..0x73ba0. */
int prefix_character(char *characters, char c)
{
    if (characters)
        while (*characters)
            if (*characters++ == c)
                return 1;
    return 0;
}


/* Original 0x73960..0x73adb. */
void apply_option(OptionRule *rule, char *value, int copy)
{
    ZRule *z;
    char *pattern;
    unsigned length;
    switch (rule->action) {
    case 1:
        *(int *)rule->target = 1;
        break;
    case 5:
        *(int *)rule->target = 0;
        break;
    case 0x24:
        *(int *)rule->target = parse_option_number(value);
        break;
    case 0x26:
        append_vector(*(Vector **)rule->target, copy ? _strdup(value) : value);
        break;
    case 0x27:
        if (*(char **)rule->target) {
            pattern = _strdup(rule->pattern);
            for (length = 0; pattern[length]; ++length)
                ;
            pattern[length - 1] = 0;
            fatal_error(46, pattern, *(char **)rule->target, value);
            break;
        }
        /* fall through */
    case 0x22:
        *(char **)rule->target = copy ? _strdup(value) : value;
        break;
    case 0x23:
        for (; *value; ++value) {
            z = (ZRule *)rule->target;
            while (z->letter && (char)z->letter != *value)
                ++z;
            if (!z->letter)
                fatal_error(48, (int)*value, current_option_argument);
            else if (z->action == 1)
                *z->target = 1;
            else if (z->action == 5)
                *z->target = 0;
        }
        break;
    }
}


/* Original 0x73ae0..0x73b7d. */
int parse_option_number(char *value)
{
    char *number = value;
    long result = 0;
    while (isspace(*number))
        ++number;
    if (isdigit(*number)) {
        result = atol(number);
        if (result >= 65535 || result < 0)
            fatal_error(49, value);
    } else
        fatal_error(49, value);
    return result;
}


/* Original 0x73ba0..0x73bfa. */
int match_option_suffix(char *pattern, char *argument, unsigned action)
{
    char *pattern_end = pattern;
    char *argument_end = argument;
    if (!pattern[1])
        return 1;
    while (pattern_end[1])
        ++pattern_end;
    while (argument_end[1])
        ++argument_end;
    while (*pattern_end == *argument_end) {
        --pattern_end;
        --argument_end;
    }
    return pattern_end == pattern && argument_end >= argument;
}


int option_Au, option_Aw; /* original 0x91380/0x913a0 */

/* Original 0x3d220..0x3d2c0; dispatch table 0x3d2c0..0x3d310. */
void finalize_options(void)
{
    char *p;
    if (!options.option_A)
        options.option_A = "sn";
    for (p = options.option_A; *p; ++p) {
        switch (*p) {
        case 'd':
        case 'n':
        case 's':
            break;
        case 'h':
            options.configuration.option_M |= 4;
            /* fall through */
        case 'f':
            options.configuration.option_M |= 2;
            break;
        case 'l':
            options.configuration.option_M |= 1;
            break;
        case 'u':
            option_Au = 1;
            options.configuration.option_M |= 8;
            break;
        case 'w':
            option_Aw = 1;
            options.configuration.option_M |= 16;
            break;
        default:
            compiler_warning(1, 126, (int)*p);
            break;
        }
    }
}
