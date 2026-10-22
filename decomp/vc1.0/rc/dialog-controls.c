/* Dispatch and serialization retain the original per-control function graph. */
#include "dialog.h"
BOOL control_consumed;

/* Original 0x0100b530..0x0100b7f5, switch table through 0x0100b864. */
void __cdecl parse_dialog_controls(void)
{
    struct DialogControl control;
    next_token(1);
    while (current_token.kind != TOKEN_END) {
        control_consumed = FALSE;
        control.style = 0x50000000;
        control.extended_style = 0;
        control.text[0] = control.text_is_ordinal = 0;
        switch (current_token.kind) {
        case TOKEN_RADIOBUTTON:
            control.style = control_style(4);
            parse_button_control(&control); break;
        case TOKEN_CHECKBOX:
            control.style = control_style(2) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_PUSHBUTTON:
            control.style = control_style(0) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_DEFPUSHBUTTON:
            control.style = control_style(1) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_AUTOCHECKBOX:
            control.style = control_style(3) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_STATE3:
            control.style = control_style(5) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_AUTO3STATE:
            control.style = control_style(6) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_USERBUTTON:
            control.style = control_style(8) | 0x10000;
            parse_button_control(&control); break;
        case TOKEN_AUTORADIOBUTTON:
            control.style = control_style(9);
            parse_button_control(&control); break;
        case TOKEN_GROUPBOX:
            control.style = control_style(7);
            parse_group_control(&control); break;
        case TOKEN_EDITTEXT:
            parse_edit_control(&control); break;
        case TOKEN_LTEXT:
            control.style = control_style(0);
            parse_static_control(&control); break;
        case TOKEN_RTEXT:
            control.style = control_style(2);
            parse_static_control(&control); break;
        case TOKEN_CTEXT:
            control.style = control_style(1);
            parse_static_control(&control); break;
        case TOKEN_ICON:
            control.style = control_style(3);
            parse_icon_control(&control); break;
        case TOKEN_LISTBOX:
            control.style = control_style(0x800001);
            parse_listbox_control(&control); break;
        case TOKEN_COMBOBOX:
            parse_combobox_control(&control); break;
        case TOKEN_SCROLLBAR:
            parse_scrollbar_control(&control); break;
        case TOKEN_GENERIC_CONTROL:
            parse_generic_control(&control); break;
        default:
            parser_error("Invalid Control type", "");
            return;
        }
        if (!control_consumed) {
            next_token(1);
            combine_word_operators(&control.cy);
        }
        put_dialog_control(&control);
    }
}

/* Original 0x0100b3f0..0x0100b3fc. */
DWORD __stdcall control_style(DWORD style)
{
    return style | 0x50000000;
}

/* Original 0x0100ac40..0x0100acfb. */
void __stdcall put_dialog_control(struct DialogControl *control)
{
    align_resource_data();
    put_dword(control->style);
    put_dword(control->extended_style);
    put_word(control->x);
    put_word(control->y);
    put_word(control->cx);
    put_word(control->cy);
    put_word(control->command);
    if (control->class_name[0] == 0xffff) {
        put_word(0xffff);
        put_word(control->class_name[1]);
    } else
        put_wide_string(control->class_name);
    if (control->text_is_ordinal) {
        put_word(0xffff);
        put_word((WORD)wide_atoi(control->text));
    } else
        put_wide_string(control->text);
    end_dialog_control();
    increment_dialog_controls();
}

/* Original 0x0100ad00..0x0100ad08. */
void __cdecl end_dialog_control(void)
{
    put_word(0);
}
/* Original 0x0100ad10..0x0100ad20. */
void __cdecl increment_dialog_controls(void)
{
    ++*(WORD *)(resource_data + dialog_control_count_offset);
}
