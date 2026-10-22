/* Dialog coordinates, metadata and template emission. */
#include "dialog.h"
unsigned int dialog_control_count_offset;

/* Original 0x0100b8c0..0x0100ba21. */
BOOL __stdcall parse_dialog(struct ResourceInfo *resource)
{
    struct DialogTemplate *dialog = active_dialog;
    dialog->extended_style = resource->extended_style;
    dialog->style = 0x80880000;
    dialog->menu[0] = dialog->class_name[0] = dialog->caption[0] = 0;
    dialog->menu_is_ordinal = dialog->class_is_ordinal = 0;
    dialog->point_size = 0;
    dialog->x = parse_word_expression();
    advance_argument();
    combine_word_operators(&dialog->x);
    dialog->y = parse_word_expression();
    advance_argument();
    combine_word_operators(&dialog->y);
    dialog->cx = parse_word_expression();
    advance_argument();
    combine_word_operators(&dialog->cx);
    dialog->cy = parse_word_expression();
    advance_argument();
    combine_word_operators(&dialog->cy);
    parse_dialog_options(resource);
    if (dialog->point_size)
        dialog->style |= 0x40;
    else
        dialog->style &= ~0x40;
    put_dialog_header(dialog);
    if (current_token.kind != TOKEN_BEGIN)
        parser_error("BEGIN expected in Dialog", "");
    parse_dialog_controls();
    if (current_token.kind != TOKEN_END)
        parser_error("END expected in Dialog", "");
    return TRUE;
}

/* Original 0x0100ab60..0x0100ac3d. */
void __stdcall put_dialog_header(struct DialogTemplate *dialog)
{
    put_dword(dialog->style);
    put_dword(dialog->extended_style);
    dialog_control_count_offset = resource_data_size;
    put_word(0);
    put_word(dialog->x);
    put_word(dialog->y);
    put_word(dialog->cx);
    put_word(dialog->cy);
    if (dialog->menu_is_ordinal) {
        put_word(0xffff);
        put_word((WORD)wide_atoi(dialog->menu));
    } else
        put_wide_string(dialog->menu);
    if (dialog->class_is_ordinal) {
        put_word(0xffff);
        put_word((WORD)wide_atoi(dialog->class_name));
    } else
        put_wide_string(dialog->class_name);
    put_wide_string(dialog->caption);
    if (dialog->point_size) {
        put_word(dialog->point_size);
        put_wide_string(dialog->font);
    }
}

/* Original 0x0100c190..0x0100c2ea. EXSTYLE retains the original full-word
 * comparison even though the lexer stores a byte token kind. */
void __stdcall parse_dialog_options(struct ResourceInfo *resource)
{
    unsigned int token;
    while (current_token.kind != TOKEN_BEGIN) {
        token = current_token.kind;
        switch (token) {
        case 70: parse_dialog_menu(); break;
        case 94: parse_dialog_font(); break;
        case 100: parse_dialog_class(); break;
        case 130:
            next_token(0);
            if (current_token.kind != TOKEN_NUMBER)
                parser_error("CHARACTERISTICS not a number", "");
            resource->characteristics = current_token.value;
            next_token(0);
            break;
        case 131:
            resource->language = parse_language();
            next_token(0);
            break;
        case 132:
            next_token(0);
            if (current_token.kind != TOKEN_NUMBER)
                parser_error("VERSION not a number", "");
            resource->version = current_token.value;
            next_token(0);
            break;
        case 133: active_dialog->style = parse_style(0); break;
        case 135: parse_dialog_caption(); break;
        case 65527: active_dialog->extended_style = parse_style(0); break;
        default: parser_error("BEGIN expected in dialog", "");
        }
    }
}
