/* Built-in Windows control classes preserve their individual RC parsers. */
#include "dialog.h"
#include <recon.h>

/* Original 0x0100bc90..0x0100bcd8. */
void __stdcall parse_button_control(struct DialogControl *control)
{
    DWORD initial = control->style;
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x80;
    parse_caption_control(control);
    if ((initial ^ control->style) & 0xffff)
        fprintf(diagnostics, "\n%s (%d) : warning RW4002: Redefinition of Button Type\n",
                source_filename, current_token.line);
}
/* Original 0x0100bce0..0x0100bcf9. */
void __stdcall parse_group_control(struct DialogControl *control)
{
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x80;
    parse_caption_control(control);
}
/* Original 0x0100bd00..0x0100bd1f. */
void __stdcall parse_static_control(struct DialogControl *control)
{
    control->style |= 0x20000;
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x82;
    parse_caption_control(control);
}
/* Original 0x0100bd20..0x0100bd39. */
void __stdcall parse_icon_control(struct DialogControl *control)
{
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x82;
    parse_caption_control(control);
}
/* Original 0x0100bd40..0x0100bda1. */
void __stdcall parse_listbox_control(struct DialogControl *control)
{
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x83;
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
/* Original 0x0100bdb0..0x0100be11. */
void __stdcall parse_scrollbar_control(struct DialogControl *control)
{
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x84;
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
/* Original 0x0100be20..0x0100be81. */
void __stdcall parse_combobox_control(struct DialogControl *control)
{
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x85;
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
/* Original 0x0100bef0..0x0100bf6b. */
void __stdcall parse_edit_control(struct DialogControl *control)
{
    control->class_name[0] = 0xffff;
    control->class_name[1] = 0x81;
    control->style = control_style(0x800000) | 0x10000;
    control->text[0] = 0;
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
