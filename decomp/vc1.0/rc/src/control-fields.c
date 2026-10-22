/* Shared dialog-control text, ID and coordinate readers. */
#include "dialog.h"
#include <string.h>
#include <recon.h>
#pragma intrinsic(memcpy)
WORD __stdcall advance_argument_flags(WORD);

/* Original 0x0100bf70..0x0100c0b5. */
void __stdcall parse_control_text(struct DialogControl *control)
{
    advance_argument_flags(TOKEN_LITERAL_ONLY);
    if (current_token.kind == TOKEN_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Control Text String too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.spelling[256] = 0;
            current_token.length = 255;
        }
        ansi_to_wide(control->text, current_token.spelling, current_token.length + 1);
        control->text_is_ordinal = 0;
    } else if (current_token.kind == TOKEN_WIDE_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Control Text String too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.text[256] = 0;
            current_token.length = 255;
        }
        memcpy(control->text, current_token.text, current_token.length * 2 + 2);
        control->text_is_ordinal = 0;
    } else if (current_token.kind == TOKEN_NUMBER) {
        wide_itoa(current_token.length, control->text, 10);
        control->text_is_ordinal = 1;
    } else
        parser_error("Text string or ordinal expected in Control", "");
}

/* Original 0x0100c0c0..0x0100c0fd. */
void __stdcall parse_control_id(struct DialogControl *control)
{
    advance_argument();
    if (current_token.kind != TOKEN_NUMBER)
        parser_error("Expecting number for ID", "");
    control->command = current_token.length;
    advance_argument();
    combine_word_operators(&control->command);
}

/* Original 0x0100c130..0x0100c186. The final coordinate remains current. */
void __stdcall parse_control_rectangle(struct DialogControl *control)
{
    control->x = parse_word_expression();
    advance_argument();
    combine_word_operators(&control->x);
    control->y = parse_word_expression();
    advance_argument();
    combine_word_operators(&control->y);
    control->cx = parse_word_expression();
    advance_argument();
    combine_word_operators(&control->cx);
    control->cy = parse_word_expression();
}

/* Original 0x0100be90..0x0100beeb. */
void __stdcall parse_caption_control(struct DialogControl *control)
{
    parse_control_text(control);
    parse_control_id(control);
    parse_control_rectangle(control);
    next_token(1);
    combine_word_operators(&control->cy);
    if (current_token.kind == TOKEN_COMMA) {
        control->style = parse_style(control->style);
        if (current_token.kind == TOKEN_COMMA)
            control->extended_style = parse_style(0);
    }
    control_consumed = TRUE;
}
