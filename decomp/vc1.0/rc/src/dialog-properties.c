/* FONT, MENU, CLASS and CAPTION properties of the active dialog scratch object. */
#include "dialog.h"
#include <string.h>
#include <recon.h>
#pragma intrinsic(memcpy)

/* Original 0x0100c2f0..0x0100c42b. */
void __cdecl parse_dialog_font(void)
{
    next_token(1);
    if (current_token.kind == TOKEN_NUMBER)
        active_dialog->point_size = current_token.length;
    else
        parser_error("Expected numeric point size", "");
    advance_argument();
    if (current_token.kind == TOKEN_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Font Name too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.spelling[256] = 0;
            current_token.length = 255;
        }
        ansi_to_wide(active_dialog->font, current_token.spelling, current_token.length + 1);
    } else if (current_token.kind == TOKEN_WIDE_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Font Name too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.text[256] = 0;
            current_token.length = 255;
        }
        memcpy(active_dialog->font, current_token.text, current_token.length * 2 + 2);
    } else
        parser_error("Expected font face name", "");
    next_token(1);
}

/* Original 0x0100c430..0x0100c4b4. read_identifier leaves the prior token's
 * WORD length intact; preserve its use as the conversion count here. */
void __cdecl parse_dialog_menu(void)
{
    char *name = read_identifier();
    if (current_token.length > 256) {
        fprintf(diagnostics, "\n%s (%d) : warning RW4006: Menu Name String too long - truncated at 256\n",
                source_filename, current_token.line);
        current_token.spelling[256] = 0;
        current_token.length = 255;
    }
    ansi_to_wide(active_dialog->menu, name, current_token.length + 1);
    if (active_dialog->menu[0] >= '0' && active_dialog->menu[0] <= '9')
        active_dialog->menu_is_ordinal = TRUE;
    next_token(1);
}

/* Original 0x0100c4c0..0x0100c5df. */
void __cdecl parse_dialog_caption(void)
{
    next_token(1);
    if (current_token.kind == TOKEN_WIDE_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Title String too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.text[256] = 0;
            current_token.length = 255;
        }
        memcpy(active_dialog->caption, current_token.text, current_token.length * 2 + 2);
        active_dialog->style |= 0xc00000;
    } else if (current_token.kind == TOKEN_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Title String too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.spelling[256] = 0;
            current_token.length = 255;
        }
        ansi_to_wide(active_dialog->caption, current_token.spelling, current_token.length + 1);
        active_dialog->style |= 0xc00000;
    } else
        parser_error("Expecting quoted string in dialog title", "");
    next_token(1);
}

/* Original 0x0100c5e0..0x0100c729. */
void __cdecl parse_dialog_class(void)
{
    next_token(1);
    if (current_token.kind == TOKEN_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Class Name String too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.spelling[256] = 0;
            current_token.length = 255;
        }
        ansi_to_wide(active_dialog->class_name, current_token.spelling, current_token.length + 1);
    } else if (current_token.kind == TOKEN_WIDE_STRING) {
        if (current_token.length > 256) {
            fprintf(diagnostics, "\n%s (%d) : warning RW4006: Class Name String too long - truncated at 256\n",
                    source_filename, current_token.line);
            current_token.text[256] = 0;
            current_token.length = 255;
        }
        memcpy(active_dialog->class_name, current_token.text, current_token.length * 2 + 2);
    } else if (current_token.kind == TOKEN_NUMBER) {
        wide_itoa(current_token.length, active_dialog->class_name, 10);
        active_dialog->class_is_ordinal = TRUE;
    } else
        parser_error("Expecting quoted string in dialog class", "");
    next_token(1);
}
