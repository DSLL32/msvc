/* String constants used in expressions, #line and include directives. */
#include "directives.h"
/* Original [0100c390,0100c4b3). Include-name mode preserves backslashes;
 * ordinary constants translate escapes. The target warns once and discards
 * excess characters after the shared string buffer's effective limit. */
void rc_parse_string(void)
{
    char *out = rc_text_buffer;
    unsigned char ch;
    unsigned status;
    int warn = 1;
    rc_token_value = rc_text_buffer;
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_END:
            status = rc_resume_input();
            if (status != 16) continue;
            if (!rc_header_mode) ch = (unsigned char)rc_escape_value(rc_get_logical_character());
            break;
        case LC_NEWLINE:
            --rc_input_cursor;
            rc_message_format = rc_message(2001);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2001);
            goto finished;
        case LC_STRING:
            goto finished;
        case LC_DBCS_LEAD:
            *out++ = ch;
            ch = rc_get_logical_character();
            break;
        }
        if (out - rc_text_buffer > 2043) {
            if (warn) {
                rc_message_format = rc_message(4009);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_warning(4009);
                warn = 0;
            }
        } else *out++ = ch;
    }
finished:
    *out++ = 0;
    rc_token_string_length = (unsigned short)(out - (char *)rc_token_value);
}
