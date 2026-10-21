/* Semantic option defaults; storage follows the driver options they control. */
#include "preprocessor.h"
RcOptions rc_options = {
    1,0,0,0,0,0,
    0,0,0,0,0,
    2,0,0,0,0,1,
    0,1,0,0,
    "/",NULL,NULL,NULL,NULL,
    "c1.err","",NULL,NULL,NULL,NULL
};
RcStringOptions rc_definitions = { 100 };
RcStringOptions rc_include_directories = { 100 };
int rc_conditional_depth = -1;

char *rc_current_option;
/* Original [01007440,0100753c). '#' captures an attached or following value;
 * '*' matches a nonempty value ending in the rest of the pattern. */
int __stdcall rc_parse_option(RcOption *options, char *argument,
                              char *(__cdecl *next_argument)(void), int copy)
{
    const char *pattern;
    char *value;
    if (!argument) return 0;
    rc_current_option = argument;
    for (; options->action; ++options) {
        pattern = options->pattern;
        value = argument;
        for (;;) {
            if (!*pattern) {
                if (*value) break;
                rc_apply_option(options, options->action & 0x20 ? next_argument() : NULL, copy);
                return options->again;
            }
            if (*pattern == '#') {
                rc_apply_option(options, *value ? value : next_argument(), copy);
                return options->again;
            }
            if (*pattern == '*') {
                if (*value && rc_match_suffix(pattern, value)) {
                    rc_apply_option(options, value, copy);
                    return options->again;
                }
                break;
            }
            if (*value++ != *pattern++) break;
        }
    }
    return 0;
}

#include <string.h>
/* Original [01007540,01007763). The table defines flag, scalar, bounded list,
 * single string, and single-letter group actions. */
void __stdcall rc_apply_option(RcOption *option, char *value, int copy)
{
    RcStringOptions *list;
    RcSuboption *sub;
    char *pattern, *end;
    switch (option->action) {
    case 1:
        *(int *)option->destination = 1;
        return;
    case 5:
        *(int *)option->destination = 0;
        return;
    case 0x24:
        *(int *)option->destination = rc_parse_option_number(value);
        return;
    case 0x26:
        list = option->destination;
        if (list->remaining > 0) {
            --list->remaining;
            list->values[list->remaining] = copy ? strdup(value) : value;
            return;
        }
        rc_message_format = rc_message(1047);
        rc_format(rc_diagnostic_buffer, rc_message_format, option->pattern, value);
        rc_fatal(1047);
        return;
    case 0x27:
        if (*(char **)option->destination) {
            pattern = strdup(option->pattern);
            for (end = pattern; *end; ++end) ;
            end[-1] = 0;
            rc_message_format = rc_message(1046);
            rc_format(rc_diagnostic_buffer, rc_message_format, pattern,
                      *(char **)option->destination, value);
            rc_fatal(1046);
            return;
        }
        /* Otherwise this is the same string assignment as action 0x22. */
    case 0x22:
        *(char **)option->destination = copy ? strdup(value) : value;
        return;
    case 0x23:
        while (*value) {
            for (sub = option->destination; sub->letter; ++sub)
                if ((char)sub->letter == *value) break;
            if (sub->letter) {
                if (sub->action == 1) *sub->destination = 1;
                else if (sub->action == 5) *sub->destination = 0;
            } else {
                rc_message_format = rc_message(1048);
                rc_format(rc_diagnostic_buffer, rc_message_format,
                          (int)*value, rc_current_option);
                rc_fatal(1048);
            }
            ++value;
        }
    }
}
