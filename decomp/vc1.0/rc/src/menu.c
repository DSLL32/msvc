/* Nested Windows menu templates. */
#include "menu.h"
#include <string.h>
#pragma intrinsic(memcpy)
unsigned int last_resource_item_offset;

/* Original 0x0100c7d0..0x0100c970. */
BOOL __stdcall parse_menu(int nested, struct ResourceInfo *resource)
{
    BOOL have_items = FALSE;
    unsigned int saved;
    if (!nested && current_token.kind != TOKEN_BEGIN) {
        do {
            switch (current_token.kind) {
            case 130:
                next_token(0);
                if (current_token.kind != TOKEN_NUMBER)
                    parser_error("CHARACTERISTICS not a number", "");
                resource->characteristics = current_token.value;
                break;
            case 131:
                resource->language = parse_language();
                break;
            case 132:
                next_token(0);
                if (current_token.kind != TOKEN_NUMBER)
                    parser_error("VERSION not a number", "");
                resource->version = current_token.value;
                break;
            default:
                parser_error("BEGIN expected in menu", "");
            }
            next_token(0);
        } while (current_token.kind != TOKEN_BEGIN);
    }
    if (current_token.kind != TOKEN_BEGIN)
        parser_error("BEGIN expected in menu", "");
    next_token(1);
    while (current_token.kind != TOKEN_END) {
        switch (current_token.kind) {
        case 71: /* MENUITEM */
            have_items = TRUE;
            parse_menu_item(FALSE);
            break;
        case 78: /* POPUP */
            have_items = TRUE;
            parse_menu_item(TRUE);
            saved = last_resource_item_offset;
            parse_menu(1, resource);
            last_resource_item_offset = saved;
            break;
        default:
            parser_error("Unknown Menu SubType", "");
        }
    }
    if (current_token.kind != TOKEN_END)
        parser_error("END expected in menu", "");
    if (!have_items)
        parser_error("Empty menus not allowed", "");
    if (nested)
        next_token(1);
    finish_menu();
    return TRUE;
}

/* Original 0x0100ad20..0x0100ad2b. */
void __cdecl mark_menu_item(void)
{
    last_resource_item_offset = resource_data_size;
}
/* Original 0x0100ad30..0x0100ad40. */
void __cdecl finish_menu(void)
{
    resource_data[last_resource_item_offset] |= 0x80;
}
/* Original 0x0100ad60..0x0100adb0. */
void __stdcall put_menu_item(struct MenuItem *item)
{
    WCHAR *text = item->text;
    put_word(item->flags);
    if (!(item->flags & 0x10))
        put_word(item->command);
    while (*text)
        put_word(*text++);
    put_word(0);
}

/* Original 0x0100c970..0x0100cb46. */
void __stdcall parse_menu_item(BOOL popup)
{
    struct MenuItem item;
    item.popup = (BYTE)popup;
    next_token(1);
    if (current_token.kind == TOKEN_STRING || current_token.kind == TOKEN_WIDE_STRING) {
        if (current_token.kind == TOKEN_WIDE_STRING) {
            if (current_token.length > 256) {
                fprintf(diagnostics, "\n%s (%d) : warning RW4006: Menu Text String too long - truncated at 256\n",
                        source_filename, current_token.line);
                current_token.text[256] = 0;
                current_token.length = 255;
            }
            memcpy(item.text, current_token.text, current_token.length * 2 + 2);
        } else {
            if (current_token.length > 256) {
                fprintf(diagnostics, "\n%s (%d) : warning RW4006: Menu Text String too long - truncated at 256\n",
                        source_filename, current_token.line);
                current_token.spelling[256] = 0;
                current_token.length = 255;
            }
            ansi_to_wide(item.text, current_token.spelling, current_token.length + 1);
        }
        item.flags = 0x10;
        if (!popup) {
            item.flags = 0;
            advance_argument();
            if (current_token.kind != TOKEN_NUMBER)
                parser_error("Expected ID value for Menuitem", "");
            item.command = current_token.length;
            advance_argument();
            combine_word_operators(&item.command);
        } else
            advance_argument();
        while (parse_menu_flag(current_token.kind, &item))
            advance_argument();
    } else if (current_token.kind == 72) { /* SEPARATOR */
        item.text[0] = item.command = 0;
        item.flags = 0;
        advance_argument();
    } else
        parser_error("Expected Menu String", "");
    mark_menu_item();
    put_menu_item(&item);
}

/* Original 0x0100cb50..0x0100cbe8, switch table through 0x0100cc60.
 * OWNERDRAW and HELP are recognized but leave the byte flags unchanged. */
BOOL __stdcall parse_menu_flag(unsigned int token, struct MenuItem *item)
{
    switch (token) {
    case 67: item->flags |= 4; break;  /* BITMAP */
    case 73: item->flags |= 8; break;  /* CHECKED */
    case 74: item->flags |= 1; break;  /* GRAYED */
    case 75: item->flags |= 2; break;  /* INACTIVE */
    case 76: item->flags |= 32; break; /* MENUBARBREAK */
    case 77: item->flags |= 64; break; /* MENUBREAK */
    case 50: case 79: break;
    default: return FALSE;
    }
    return TRUE;
}
