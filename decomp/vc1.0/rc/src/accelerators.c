/* Accelerator table parsing and the original separate end-marker functions. */
#include "menu.h"
#include <ctype.h>
#include <string.h>
#include <recon.h>
#pragma intrinsic(strlen)
void __stdcall accelerator_word_operators(WORD *);
void __cdecl mark_accelerator(void);
void __cdecl finish_accelerators(void);

/* Original 0x0100d6f0..0x0100db67. Its unused result retains the legacy
 * five-byte entry count even though each NT entry emits four WORDs. */
unsigned int __stdcall parse_accelerators(struct ResourceInfo *resource)
{
    unsigned int count = 0;
    BYTE kind;
    char control_character;
    WORD flags, key, command, upper;
    BOOL specified_kind;
    if (current_token.kind != TOKEN_BEGIN) {
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
                parser_error("BEGIN expected in Accelerator Table", "");
            }
            next_token(0);
        } while (current_token.kind != TOKEN_BEGIN);
        if (current_token.kind != TOKEN_BEGIN)
            parser_error("BEGIN expected in Accelerator Table", "");
    }
    next_token(1);
    while (current_token.kind != TOKEN_END) {
        specified_kind = FALSE;
        kind = current_token.kind;
        if (kind == TOKEN_NUMBER)
            key = current_token.length;
        else if (kind == TOKEN_STRING) {
            if (current_token.spelling[0] == '^') {
                if (strlen(current_token.spelling) != 2)
                    parser_error("Invalid control character", "");
                control_character = (char)rc_toupper(current_token.spelling[1]);
                if (!isalpha(control_character))
                    parser_error("Control Character out of range [^A - ^Z]", "");
                key = (WORD)(control_character - '@');
            } else if (strlen(current_token.spelling) == 2)
                key = (WORD)(((WORD)(signed char)current_token.spelling[0] << 8) +
                             (signed char)current_token.spelling[1]);
            else if (strlen(current_token.spelling) == 1)
                key = (WORD)current_token.spelling[0];
            else
                parser_error("Invalid Accelerator", "");
        } else
            parser_error("Expected String or Constant Accelerator command", "");
        next_token(1);
        if (current_token.kind != TOKEN_COMMA)
            parser_error("Expected Comma in Accelerator Table", "");
        next_token(1);
        if (current_token.kind != TOKEN_NUMBER)
            parser_error("Expected numeric command value", "");
        command = current_token.length;
        next_token(1);
        accelerator_word_operators(&command);
        flags = 0;
        while (current_token.kind == TOKEN_COMMA) {
            next_token(1);
            switch (current_token.kind) {
            case 103: flags |= 2; break;  /* NOINVERT */
            case 106: flags |= 4; break;  /* SHIFT */
            case 110: flags |= 16; break; /* ALT */
            case 111: specified_kind = TRUE; break; /* ASCII */
            case 112: flags |= 1; specified_kind = TRUE; break; /* VIRTKEY */
            case 134: flags |= 8; break; /* CONTROL */
            default: parser_error("Invalid Accelerator Type [ASCII or VIRTKEY]", "");
            }
            next_token(1);
        }
        if (kind == TOKEN_NUMBER && !specified_kind)
            parser_error("Accelerator Type required [ASCII or VIRTKEY]", "");
        if (!(flags & 1) && (flags & 12))
            fprintf(diagnostics, "\n%s (%d) : warning RW4003: SHIFT or CONTROL used without VIRTKEY\n",
                    source_filename, current_token.line);
        if ((flags & 1) && kind == TOKEN_STRING) {
            upper = (WORD)rc_toupper(key);
            if (!isalnum(upper))
                fprintf(diagnostics, "\n%s (%d) : warning RW4004: ASCII character not equivalent to virtual key code\n",
                        source_filename, current_token.line);
        }
        mark_accelerator();
        put_word(flags);
        put_word(key);
        put_word(command);
        put_word(0);
        ++count;
    }
    finish_accelerators();
    return count * 5;
}

/* Original 0x0100d690..0x0100d6e6. This variant also accepts subtraction. */
void __stdcall accelerator_word_operators(WORD *value)
{
    BYTE operator;
    for (;;) {
        operator = current_token.kind;
        if (operator != TOKEN_PLUS && operator != TOKEN_MINUS && operator != TOKEN_OR)
            break;
        next_token(1);
        if (operator == TOKEN_PLUS)
            *value += parse_word_expression();
        else if (operator == TOKEN_MINUS)
            *value -= parse_word_expression();
        else
            *value |= parse_word_expression();
        next_token(1);
    }
}

/* Original 0x0100ad40..0x0100ad4b. Shared storage, distinct graph vertex. */
void __cdecl mark_accelerator(void)
{
    last_resource_item_offset = resource_data_size;
}
/* Original 0x0100ad50..0x0100ad60. */
void __cdecl finish_accelerators(void)
{
    resource_data[last_resource_item_offset] |= 0x80;
}
