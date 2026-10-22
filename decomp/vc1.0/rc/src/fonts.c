/* Font-file directory extraction. The resource itself remains in its input
 * stream; a linked directory list is emitted after ordinary resources. */
#include "fonts.h"
#include "resource-data.h"
#include <string.h>
#include <stddef.h>
#pragma intrinsic(memcpy)
struct FontDirectoryEntry *first_font, *last_font;

/* Original 0x0100af70..0x0100b0df. */
BOOL __stdcall parse_font(struct ResourceInfo *resource)
{
    struct {
        struct FontDirectoryHeader header;
        BYTE names[64];
    } directory;
    struct FontDirectoryEntry *font, *previous;
    FILE *file = resource_stream(0);
    BYTE *next = directory.names;
    int length;
    seek_file(file, 0, SEEK_SET);
    read_file(file, &directory.header, sizeof(directory.header));
    if (directory.header.device_offset) {
        seek_file(file, directory.header.device_offset, SEEK_SET);
        do {
            read_file(file, next, 1);
        } while (*next++);
    } else
        *next++ = 0;
    seek_file(file, directory.header.face_offset, SEEK_SET);
    do {
        read_file(file, next, 1);
    } while (*next++);
    length = (short)(next - (BYTE *)&directory);
    font = resource_alloc(length + offsetof(struct FontDirectoryEntry, directory));
    font->ordinal = resource->ordinal;
    font->length = (WORD)length;
    font->next = 0;
    memcpy(font->directory, &directory, length);
    if (!font_count)
        first_font = last_font = font;
    else {
        for (previous = first_font; previous; previous = previous->next) {
            if (previous->ordinal == font->ordinal) {
                fprintf(diagnostics, "\n%s (%d) : error RW2004: Duplicate Font ID %d\n",
                        source_filename, current_token.line, font->ordinal);
                resource_free(font);
                return FALSE;
            }
        }
        last_font->next = font;
        last_font = last_font->next;
    }
    seek_file(file, 0, SEEK_SET);
    return TRUE;
}

/* Original 0x01009de0..0x01009e67. */
void __cdecl emit_font_directory(void)
{
    struct ResourceInfo *resource = resource_alloc(sizeof(*resource));
    struct ResourceType *type;
    struct FontDirectoryEntry *font;
    resource->language = (WORD)default_language;
    resource->version = default_resource_version;
    resource->characteristics = default_characteristics;
    resource->name = duplicate_wide(L"FONTDIR");
    type = find_resource_type(0, 7);
    begin_resource_data();
    put_word(font_count);
    for (font = first_font; font; font = font->next) {
        put_word(font->ordinal);
        put_bytes(font->directory, font->length);
    }
    resource->memory_flags = 0x50;
    write_resource(type, resource);
}
