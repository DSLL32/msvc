/* Comment handling separates forced discarding from the /C-preserving paths. */
#include "directives.h"
/* Original [0100ce00,0100ce27). Failed matching puts one logical character
 * back; any skipped CRs and continued lines remain consumed. */
int __stdcall rc_match_character(int expected)
{
    if (rc_get_logical_character() == expected) return 1;
    --rc_input_cursor;
    return 0;
}

/* Original [0100cf50,0100cf7f). Used where comments must be whitespace even
 * when /C preserves comments in ordinary scanner output. */
int rc_skip_comment(void)
{
    if (rc_match_character('*')) {
        rc_discard_block_comment();
        return 1;
    }
    if (rc_match_character('/')) {
        rc_discard_line_comment();
        return 1;
    }
    return 0;
}

/* Original [0100ce30,0100ce9e). Its non-/C path is a tail jump to the same
 * discard-line vertex called by rc_skip_comment. */
void rc_skip_line_comment(void)
{
    unsigned char ch;
    if (!rc_options.keep_comments) { rc_discard_line_comment(); return; }
    fwrite("//", 2, 1, rc_output);
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_CR: break;
        case LC_END: rc_resume_input(); break;
        case LC_NEWLINE: --rc_input_cursor; return;
        default: fputc(ch, rc_output);
        }
    }
}

/* Original [0100cea0,0100cf47). Kept block comments normalize CRs and retain
 * logical newlines; the closing delimiter uses logical-character matching. */
void rc_skip_block_comment(void)
{
    unsigned char ch;
    if (!rc_options.keep_comments) { rc_discard_block_comment(); return; }
    fwrite("/*", 2, 1, rc_output);
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_CR: continue;
        case LC_END: rc_resume_input(); continue;
        case LC_STAR:
            if (rc_match_character('/')) { fwrite("*/", 2, 1, rc_output); return; }
            break;
        case LC_NEWLINE: ++rc_line_number; break;
        }
        fputc(ch, rc_output);
    }
}

/* Original [0100cf80,0100d012). Discarded comments still advance line counts
 * and emit newlines when preprocessing output is active. Star runs and buffer
 * boundaries are handled without inventing nested-comment behavior. */
void rc_discard_block_comment(void)
{
    unsigned char ch;
    for (;;) {
        ch = *rc_input_cursor++;
        if (ch == '*') {
            for (;;) {
                ch = *rc_input_cursor++;
                if (ch == '/') return;
                if (ch == '*') continue;
                if (!ch) { rc_resume_input(); continue; }
                break;
            }
        }
        if (ch == '\n') {
            ++rc_line_number;
            if (rc_preprocessing_started) fputc('\n', rc_output);
        } else if (!ch) rc_resume_input();
    }
}

/* Original [0100e340,0100e377). The newline remains for ordinary line setup. */
void rc_discard_line_comment(void)
{
    unsigned category;
    for (;;) {
        category = rc_character_class[*rc_input_cursor++];
        if (category == LC_END) rc_resume_input();
        else if (category == LC_NEWLINE) { --rc_input_cursor; return; }
    }
}
