/* Processing at the beginning of an input line. */
#include "lexer.h"
/* Original [0100c4c0,0100c5aa), jump table [0100c5ac,0100c5dc). */
void rc_begin_line(void)
{
    unsigned char ch;
    ++rc_line_number;
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_CR:
            break;
        case LC_NEWLINE:
            ++rc_line_number;
            /* Newlines share the whitespace output path. */
        case LC_SPACE:
            if (rc_preprocessing_started) fputc(rc_input_cursor[-1], rc_output);
            else {
                do ch = *rc_input_cursor++;
                while (rc_character_flags[ch] & 64);
                --rc_input_cursor;
            }
            break;
        case LC_SLASH:
            if (rc_skip_comment()) break;
            --rc_input_cursor;
            return;
        case LC_END:
            if (!ch || ch == 0x1a) {
                if (!rc_refill_input()) break;
                return;
            }
            if (rc_splice_line()) break;
            --rc_input_cursor;
            return;
        case LC_HASH:
            rc_parse_directive();
            break;
        default:
            --rc_input_cursor;
            return;
        }
    }
}
