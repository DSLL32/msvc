/* STRINGTABLE entries are collected by language and groups of sixteen IDs,
 * then emitted after the ordinary resource records. */
#include "menu.h"
#include <stdlib.h>
#include <wchar.h>

#pragma pack(2)
struct StringBlock {
    struct StringBlock *next;
    DWORD version, characteristics;
    WORD ordinal, memory_flags, language;
    WCHAR *strings[16];
};
#pragma pack()
static struct StringBlock *string_blocks;
void * __stdcall copy_resource_memory(unsigned int, void *);

/* Original 0x0100d320..0x0100d5ac. */
struct ResourceInfo * __stdcall parse_string_table(struct ResourceInfo *resource)
{
    struct StringBlock *block, *last;
    WORD identifier;
    BOOL found;
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
                parser_error("BEGIN expected in String or Error Table", "");
            }
            next_token(0);
        } while (current_token.kind != TOKEN_BEGIN);
    }
    if (current_token.kind != TOKEN_BEGIN)
        parser_error("BEGIN expected in String or Error Table", "");
    next_token(1);
    if (!string_table_resource)
        string_blocks = 0;
    do {
        found = FALSE;
        block = string_blocks;
        last = 0;
        if (current_token.kind != TOKEN_NUMBER)
            parser_error("Expected numeric constant in string table ", "");
        identifier = current_token.length;
        advance_argument();
        combine_word_operators(&identifier);
        if (current_token.kind == TOKEN_STRING)
            ansi_to_wide(current_token.text, current_token.spelling, current_token.length + 1);
        else if (current_token.kind != TOKEN_WIDE_STRING)
            parser_error("Expected String in STRINGTABLE ", "");
        while (block && !found) {
            if (block->language == resource->language && block->ordinal == (identifier >> 4)) {
                found = TRUE;
                if (!block->strings[identifier & 15])
                    block->strings[identifier & 15] = copy_resource_memory(
                        wcslen(current_token.text) * 2 + 2, current_token.text);
                else
                    parser_error("Cannot Re-use String Constants", "");
            }
            last = block;
            block = block->next;
        }
        if (!found) {
            block = copy_resource_memory(sizeof(*block), 0);
            block->ordinal = identifier >> 4;
            block->memory_flags = resource->memory_flags;
            block->language = resource->language;
            block->version = resource->version;
            block->characteristics = resource->characteristics;
            block->strings[identifier & 15] = copy_resource_memory(
                wcslen(current_token.text) * 2 + 2, current_token.text);
            if (last)
                last->next = block;
            if (!string_blocks)
                string_blocks = block;
        }
        next_token(1);
    } while (current_token.kind != TOKEN_END);
    string_table_resource = resource;
    return resource;
}

/* Original 0x0100d2b0..0x0100d312. A separate RC allocator/copy vertex. */
void * __stdcall copy_resource_memory(unsigned int size, void *source)
{
    BYTE *result = malloc(size), *next = result;
    BYTE *input = source;
    if (!result)
        resource_finish("RC : fatal error RW1025: Out of heap memory");
    if (source) {
        while (size--)
            *next++ = *input++;
    } else {
        while (size--)
            *next++ = 0;
    }
    return result;
}

/* Original 0x0100d5b0..0x0100d68c. The parameter is retained but unused. */
void __stdcall emit_string_tables(struct ResourceInfo *table)
{
    struct StringBlock *block;
    struct ResourceInfo *resource;
    struct ResourceType *type;
    WCHAR *text;
    unsigned int slot, length, size;
    for (block = string_blocks; block; block = block->next) {
        begin_resource_data();
        size = 0;
        for (slot = 0; slot != 16; ++slot) {
            text = block->strings[slot];
            length = text ? wcslen(text) : 0;
            size += length * 2 + 2;
            put_word((WORD)length);
            while (length--)
                put_word(*text++);
        }
        resource = resource_alloc(sizeof(*resource));
        resource->language = block->language;
        resource->version = block->version;
        resource->characteristics = block->characteristics;
        type = find_resource_type(0, 6);
        resource->data_size = size;
        resource->memory_flags = block->memory_flags;
        resource->ordinal = block->ordinal + 1;
        write_resource(type, resource);
    }
}
