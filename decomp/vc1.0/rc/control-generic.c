/* Original 0x0100ba30..0x0100bc89: the generic CONTROL statement. */
#include "dialog.h"
#include <string.h>

void __stdcall parse_generic_control(struct DialogControl *control)
{
    WORD builtin = 0;
    parse_control_text(control);
    parse_control_id(control);
    switch (current_token.kind) {
    case TOKEN_NUMBER:
        control->class_name[0] = (WORD)(signed char)(BYTE)current_token.length;
        control->class_name[1] = 0;
        break;
    case TOKEN_STRING:
        if (!_strcmpi(current_token.spelling, "button")) builtin = 0x80;
        else if (!_strcmpi(current_token.spelling, "edit")) builtin = 0x81;
        else if (!_strcmpi(current_token.spelling, "static")) builtin = 0x82;
        else if (!_strcmpi(current_token.spelling, "listbox")) builtin = 0x83;
        else if (!_strcmpi(current_token.spelling, "scrollbar")) builtin = 0x84;
        else if (!_strcmpi(current_token.spelling, "combobox")) builtin = 0x85;
        else {
            if (current_token.length > 256) {
                fprintf(diagnostics, "\n%s (%d) : warning RW4006: Class Name String too long - truncated at 256\n",
                        source_filename, current_token.line);
                current_token.spelling[256] = 0;
                current_token.length = 255;
            }
            ansi_to_wide(control->class_name, current_token.spelling, current_token.length + 1);
        }
        break;
    case 64: builtin = 0x81; break; /* EDIT */
    case 65: builtin = 0x82; break; /* STATIC */
    case TOKEN_LISTBOX: builtin = 0x83; break;
    case TOKEN_COMBOBOX: builtin = 0x85; break;
    case TOKEN_SCROLLBAR: builtin = 0x84; break;
    case 95: builtin = 0x80; break; /* BUTTON */
    default:
        parser_error("Expected control class name", "");
    }
    if (builtin) {
        control->class_name[0] = 0xffff;
        control->class_name[1] = builtin;
    }
    control->style = parse_style(control->style);
    advance_argument();
    parse_control_rectangle(control);
    next_token(1);
    control_consumed = TRUE;
    if (current_token.kind == TOKEN_COMMA)
        control->extended_style = parse_style(0);
}
