#include "cvtres.h"
#include <string.h>
#include <recon.h>

ResourceIdentifier *__stdcall read_identifier(FILE *file)
{
    WCHAR text[256], *end = text;
    ResourceIdentifier *identifier, **link;
    unsigned short length;
    unsigned i;

    read_bytes(file, end, sizeof(WCHAR));
    if (*end == 0xffff) {
        identifier = allocate_block(20);
        identifier->kind = 2;
        read_bytes(file, &identifier->value.ordinal, sizeof(WORD));
        return identifier;
    }
    while (*end) {
        ++end;
        read_bytes(file, end, sizeof(WCHAR));
    }
    end[1] = 0;
    length = (unsigned short)(end - text);
    link = &resource_strings;
    for (identifier = *link; identifier; identifier = *link) {
        if (!wcscmp(identifier->text, text)) return identifier;
        link = &identifier->value.next;
    }
    identifier = allocate_block(20 + length * 2);
    identifier->kind = 1;
    identifier->name_offset = resource_padding.name_bytes;
    identifier->byte_count = (unsigned short)(length * 2 + 2);
    identifier->length = length;
    for (i = 0; i < length; ++i) identifier->text[i] = text[i];
    resource_padding.name_bytes += identifier->byte_count;
    identifier->value.next = NULL;
    *link = identifier;
    return identifier;
}
