/* Header names and search order, without modern path normalization. */
#include "numbers.h"
#include "legacy-ctype.h"
#include <string.h>
/* Original [0100d3f0,0100d4e9). Leading preprocessing whitespace is skipped,
 * trailing C whitespace trimmed, and an empty <> name remains an empty name. */
char *rc_parse_angle_name(void)
{
    char *out = rc_macro_name;
    unsigned char ch = rc_read_nonspace();
    if (ch == '\n') {
        --rc_input_cursor;
        rc_message_format = rc_message(2012);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(2012);
        return NULL;
    }
    while (ch != '>') {
        if (ch == '\n') {
            --rc_input_cursor;
            rc_message_format = rc_message(2013);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2013);
            return NULL;
        }
        *out++ = ch;
        ch = rc_get_logical_character();
    }
    while (out > rc_macro_name && rc_native_isspace((signed char)out[-1])) --out;
    *out = 0;
    return rc_macro_name;
}

/* Original [0100dd70,0100de43). Values are stored from the array's high end,
 * so walking backward preserves command-line -I order. NULL and ./ mean cwd.
 * A trailing configured path separator is retained; otherwise '/' is appended. */
void rc_search_include_directories(void)
{
    int i;
    const char *directory, *cursor, *filename;
    char *out;
    for (i = 99; i >= rc_include_directories.remaining; --i) {
        directory = rc_include_directories.values[i];
        filename = rc_token_value;
        if (directory && !(directory[0] == '.' && directory[1] == '/' && !directory[2])) {
            out = rc_expansion_output;
            cursor = directory;
            do *out++ = *cursor; while (*cursor++);
            /* The original reads the byte preceding an empty directory too;
             * preserve the old spelling/terminator behavior of this routine. */
            if (!strchr(rc_options.path_characters, (signed char)cursor[-2])) out[-1] = '/';
            else --out;
            do *out++ = *filename; while (*filename++);
            filename = rc_expansion_output;
        }
        if (rc_open_include((char *)filename, 0)) return;
    }
    rc_message_format = rc_message(1015);
    rc_format(rc_diagnostic_buffer, rc_message_format, rc_token_value);
    rc_fatal(1015);
}

/* Original [01009a70,01009b41). Quoted includes try the current logical file's
 * directory, then saved enclosing filenames, before the -I list is searched. */
int rc_search_current_directory(void)
{
    const char *filename = rc_source_name, *cursor, *separator, *include_name;
    char *out;
    int level = rc_input_depth;
    for (;;) {
        separator = NULL;
        for (cursor = filename; *cursor; ++cursor) {
            if (rc_character_class[(unsigned char)*cursor] == LC_DBCS_LEAD) ++cursor;
            else if (strchr(rc_options.path_characters, (unsigned char)*cursor)) separator = cursor;
        }
        if (separator) {
            out = rc_macro_name;
            for (cursor = filename; cursor <= separator; ++cursor) *out++ = *cursor;
            include_name = rc_token_value;
            do *out++ = *include_name; while (*include_name++);
        } else rc_format(rc_macro_name, "%s", rc_token_value);
        if (rc_open_include(rc_macro_name, 0)) return 1;
        if (level <= 0) return 0;
        filename = rc_inputs[level].saved_name;
        --level;
    }
}
