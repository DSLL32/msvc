/* RCPP preprocessing initialization, original [01009d90,01009f89). */
#include "lexer.h"
#include <ctype.h>

void __stdcall rc_initialize_preprocessor(char *input_name, char *output_name,
                                          RcStringOptions *definitions)
{
    int remaining;
    char **definition;
    char *text, *source, *name, *trailing;
    unsigned length;
    rc_character_class[1] = LC_INTERNAL_1;
    rc_character_class[2] = LC_INTERNAL_2;
    rc_character_class[3] = LC_INTERNAL_3;
    rc_character_class[4] = LC_INTERNAL_4;
    if (rc_options.extensions || rc_options.zE) {
        rc_character_class['$'] = LC_IDENTIFIER;
        rc_character_flags['$'] = 16;
    }
    remaining = 100 - definitions->remaining;
    definition = definitions->values + definitions->remaining;
    while (remaining > 0) {
        source = *definition;
        name = rc_macro_name;
        *name = *source++;
        while (*name) {
            if (rc_character_class[(unsigned char)*name] == LC_DBCS_LEAD)
                *++name = *source++;
            else if (*name == '=') break;
            *++name = *source++;
        }
        if (*name == '=') {
            *name = 0;
            trailing = NULL;
            for (text = source; *text; ++text) {
                if (rc_character_class[(unsigned char)*text] == LC_DBCS_LEAD) ++text;
                else if (isspace((signed char)*text)) {
                    if (!trailing) trailing = text;
                } else trailing = NULL;
            }
            if (trailing) *trailing = 0;
            rc_macro_hash = rc_hash_name(rc_macro_name);
            for (text = rc_macro_name; *text; ++text) ;
            rc_macro_name_length = text - rc_macro_name + 1;
            if (*source) {
                for (text = source; *text; ++text) ;
                length = text - source + 2;
                rc_define_macro(source, length, -2);
            } else rc_define_macro(NULL, 0, 0);
        } else {
            rc_macro_hash = rc_hash_name(rc_macro_name);
            for (text = rc_macro_name; *text; ++text) ;
            rc_macro_name_length = text - rc_macro_name + 1;
            rc_define_macro("1\0", 3, -2);
        }
        ++definition;
        --remaining;
    }
    rc_output = fopen(output_name, "w+");
    if (!rc_output) {
        rc_message_format = rc_message(1023);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1023);
    }
    rc_open_include(input_name, 1);
}
