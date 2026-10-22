/* Fixed VERSIONINFO fields and recursive BLOCK/VALUE records. */
#include "version.h"
#include <string.h>
#pragma intrinsic(memset, strlen)

/* Original 0x0100cc60..0x0100cc7f. */
BOOL __cdecl parse_version_info(void)
{
    struct VersionHeader *header = parse_fixed_version();
    if (!header)
        return FALSE;
    header->length += parse_version_children();
    return TRUE;
}

/* Original 0x0100cc80..0x0100cd49, switch table through 0x0100cd68. */
struct VersionHeader * __cdecl parse_fixed_version(void)
{
    struct FixedVersion {
        DWORD signature, structure_version;
        DWORD file_version[2], product_version[2];
        DWORD flags_mask, flags, os, type, subtype, date[2];
    } fixed;
    memset(&fixed, 0, sizeof(fixed));
    fixed.signature = 0xfeef04bd;
    fixed.structure_version = 0x10000;
    fixed.date[0] = fixed.date[1] = 0;
    while (current_token.kind != TOKEN_BEGIN) {
        switch (current_token.kind) {
        case 120: parse_version_number(fixed.file_version); break;
        case 121: parse_version_number(fixed.product_version); break;
        case 122: parse_version_scalar(&fixed.flags_mask); break;
        case 123: parse_version_scalar(&fixed.flags); break;
        case 124: parse_version_scalar(&fixed.os); break;
        case 125: parse_version_scalar(&fixed.type); break;
        case 126: parse_version_scalar(&fixed.subtype); break;
        default:
            parser_error("Unrecognized VERSIONINFO field;", "  BEGIN or comma expected");
        }
    }
    return put_version_block("VS_VERSION_INFO", &fixed, sizeof(fixed));
}

/* Original 0x0100cd70..0x0100ce95. One through four WORD components. */
void __stdcall parse_version_number(DWORD *pair)
{
    next_token(1);
    if (current_token.kind != TOKEN_NUMBER || current_token.is_long)
        parser_error("Version WORDs separated by commas expected", "");
    pair[0] = (DWORD)current_token.length << 16;
    next_token(1);
    if (current_token.kind != TOKEN_COMMA) {
        pair[1] = 0;
        return;
    }
    next_token(1);
    if (current_token.kind != TOKEN_NUMBER || current_token.is_long)
        parser_error("Version WORDs separated by commas expected", "");
    pair[0] = (pair[0] & 0xffff0000) | current_token.length;
    next_token(1);
    if (current_token.kind != TOKEN_COMMA) {
        pair[1] = 0;
        return;
    }
    next_token(1);
    if (current_token.kind != TOKEN_NUMBER || current_token.is_long)
        parser_error("Version WORDs separated by commas expected", "");
    pair[1] = (DWORD)current_token.length << 16;
    next_token(1);
    if (current_token.kind == TOKEN_COMMA) {
        next_token(1);
        if (current_token.kind != TOKEN_NUMBER || current_token.is_long)
            parser_error("Version WORDs separated by commas expected", "");
        pair[1] = (pair[1] & 0xffff0000) | current_token.length;
        next_token(1);
    }
}

/* Original 0x0100cea0..0x0100ced5. */
void __stdcall parse_version_scalar(DWORD *value)
{
    next_token(1);
    if (current_token.kind != TOKEN_NUMBER)
        parser_error("DWORD expected", "");
    *value = current_token.value;
    next_token(1);
}

/* Original 0x0100cee0..0x0100cf8f. */
WORD __cdecl parse_version_children(void)
{
    WORD start = resource_data_length(), kind;
    struct VersionHeader *header;
    if (current_token.kind != TOKEN_BEGIN)
        parser_error("BEGIN expected in VERSIONINFO resource", "");
    next_token(1);
    for (;;) {
        kind = current_token.kind;
        if (kind == TOKEN_END)
            break;
        if (kind != 113 && kind != 114) {
            parser_error("Expected VALUE, BLOCK, or, END keyword.", "");
            continue;
        }
        next_token(1);
        if (current_token.kind != TOKEN_STRING)
            parser_error("Expecting quoted string for key", "");
        header = parse_version_value(current_token.spelling);
        if (kind == 114) {
            header->length += parse_version_children();
            next_token(1);
        }
    }
    return resource_data_length() - start;
}

/* Original 0x0100cf90..0x0100d059. */
struct VersionHeader * __stdcall put_version_block(char *key, void *value, WORD size)
{
    DWORD zero = 0;
    WORD padding, start;
    struct VersionHeader *header;
    padding = (WORD)(((resource_data_length() + 3) & 0xfffc) - resource_data_length());
    if (padding)
        put_bytes(&zero, padding);
    start = resource_data_length();
    header = (struct VersionHeader *)resource_data_end();
    put_word(0);
    put_word(size);
    put_word(0);
    ansi_to_wide(current_token.text, key, strlen(key) + 1);
    put_wide_string(current_token.text);
    if (size) {
        padding = (WORD)(((resource_data_length() + 3) & 0xfffc) - resource_data_length());
        if (padding)
            put_bytes(&zero, padding);
        put_bytes(value, size);
    }
    header->length = resource_data_length() - start;
    return header;
}

/* Original 0x0100d060..0x0100d10f. */
struct VersionHeader * __stdcall parse_version_value(char *key)
{
    DWORD zero = 0;
    WORD padding, start, *value_length, *type;
    struct VersionHeader *header;
    padding = (WORD)(((resource_data_length() + 3) & 0xfffc) - resource_data_length());
    if (padding)
        put_bytes(&zero, padding);
    start = resource_data_length();
    header = (struct VersionHeader *)resource_data_end();
    put_word(0);
    value_length = (WORD *)resource_data_end();
    put_word(0);
    type = (WORD *)resource_data_end();
    put_word(1);
    ansi_to_wide(current_token.text, key, strlen(key) + 1);
    put_wide_string(current_token.text);
    *value_length = parse_version_data(type);
    header->length = resource_data_length() - start;
    return header;
}

/* Original 0x0100d110..0x0100d251. String counts include their source NUL;
 * comma-free adjacent strings concatenate by removing the previous terminator. */
WORD __stdcall parse_version_data(WORD *type)
{
    DWORD zero = 0;
    WORD count = 0, token, padding;
    BOOL first = TRUE;
    for (;;) {
        token = advance_argument();
        if (first) {
            first = FALSE;
            padding = (WORD)(((resource_data_length() + 3) & 0xfffc) - resource_data_length());
            if (padding)
                put_bytes(&zero, padding);
        }
        switch (token) {
        case TOKEN_NUMBER:
            *type = 0;
            if (current_token.is_long) {
                put_dword(current_token.value);
                count += 4;
            } else {
                put_word(current_token.length);
                count += 2;
            }
            break;
        case TOKEN_STRING:
            if (!current_token.spelling[0])
                break;
            count += current_token.length + 1;
            ansi_to_wide(current_token.text, current_token.spelling,
                         (WORD)(current_token.length + 1));
            if (argument_had_comma)
                put_wide_string(current_token.text);
            else
                concatenate_wide_string(current_token.text);
            break;
        case TOKEN_BEGIN: case TOKEN_END: case 113: case 114:
            return count;
        default:
            parser_error("Unexpected value in value data", "");
            return 0;
        }
    }
}
