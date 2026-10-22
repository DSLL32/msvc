/* Windows NT .res records: payload, header, names and file-backed suffixes. */
#include "resource-data.h"
#include <wchar.h>

void __stdcall write_record_header(struct ResourceType *, struct ResourceInfo *, void *, long, long);
void __stdcall write_resource_name(WORD, WCHAR *);
long __cdecl resource_payload_size(void);
long __stdcall copy_resource_data(FILE *, void *, long, long);
unsigned int __stdcall copy_file_bytes(FILE *, FILE *, unsigned int);
BOOL __stdcall copy_file_to_end(FILE *, FILE *);

/* Original 0x0100b0e0..0x0100b13a. */
void __stdcall write_resource(struct ResourceType *type, struct ResourceInfo *resource)
{
    DWORD zero = 0;
    unsigned int remainder = seek_file(resource_output, 0, SEEK_CUR) & 3;
    if (remainder)
        write_file(resource_output, &zero, 4 - remainder);
    record_resource(type, resource, resource_data, resource_data_size, -1);
}

/* Original 0x01009b50..0x01009bac. */
void __stdcall record_resource(struct ResourceType *type, struct ResourceInfo *resource,
                             void *data, long size, long file_bytes)
{
    struct ResourceInfo *last = type->resources;
    if (!last)
        type->resources = resource;
    else {
        while (last->next)
            last = last->next;
        last->next = resource;
    }
    if (!error_count)
        write_record_header(type, resource, data, size, file_bytes);
    ++type->resource_count;
    ++resource_count;
}

/* Original 0x010097b0..0x01009818. */
void __stdcall write_resource_name(WORD ordinal, WCHAR *name)
{
    WORD marker = 0xffff;
    if (ordinal) {
        write_file(resource_output, &marker, 2);
        write_file(resource_output, &ordinal, 2);
    } else
        write_file(resource_output, name, wcslen(name) * 2 + 2);
}

/* Original 0x01009820..0x01009b43. The verbose type vocabulary is
 * separate from source-language keywords and preserves reserved null slots. */
void __stdcall write_record_header(struct ResourceType *type, struct ResourceInfo *resource,
                                 void *data, long size, long file_bytes)
{
    static char *type_names[] = {
        0, "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING", "FONTDIR",
        "FONT", "ACCELERATOR", "RCDATA", "MESSAGETABLE", "GROUP_CURSOR", 0,
        "GROUP_ICON", 0, "VERSION", "DLGINCLUDE"
    };
    DWORD header_size = 24, zero = 0;
    unsigned int names_size = 0;
    char numeric_type_name[256], type_name[256], resource_name[256];
    if (!resource->data_size)
        resource->data_size = resource_payload_size();
    if (!type->ordinal) {
        header_size += wcslen(type->name) * 2 + 2;
        names_size = wcslen(type->name) * 2 + 2;
    } else
        header_size += 4;
    if (!resource->ordinal) {
        header_size += wcslen(resource->name) * 2 + 2;
        names_size += wcslen(resource->name) * 2 + 2;
    } else
        header_size += 4;
    if (names_size & 3)
        header_size += 2;
    if (verbose) {
        if (!type->ordinal) {
            wide_to_ansi(type_name, type->name, (WORD)(wcslen(type->name) + 1));
            if (!resource->ordinal) {
                wide_to_ansi(resource_name, resource->name, (WORD)(wcslen(resource->name) + 1));
                fprintf(diagnostics, "\nWriting %s:%s,\tlang:0x%x,\tsize %d",
                        type_name, resource_name, resource->language, resource->data_size);
            } else
                fprintf(diagnostics, "\nWriting %s:%d,\tlang:0x%x,\tsize %d",
                        type_name, resource->ordinal, resource->language, resource->data_size);
        } else if (!resource->ordinal) {
            wide_to_ansi(numeric_type_name, resource->name, (WORD)(wcslen(resource->name) + 1));
            if (type->ordinal <= 17)
                fprintf(diagnostics, "\nWriting %s:%s,\tlang:0x%x,\tsize %d",
                        type_names[type->ordinal], numeric_type_name, resource->language, resource->data_size);
            else
                fprintf(diagnostics, "\nWriting %d:%s,\tlang:0x%x,\tsize %d",
                        type->ordinal, numeric_type_name, resource->language, resource->data_size);
        } else if (type->ordinal <= 17)
            fprintf(diagnostics, "\nWriting %s:%d,\tlang:0x%x,\tsize %d",
                    type_names[type->ordinal], resource->ordinal, resource->language, resource->data_size);
        else
            fprintf(diagnostics, "\nWriting %d:%d,\tlang:0x%x,\tsize %d",
                    type->ordinal, resource->ordinal, resource->language, resource->data_size);
    }
    write_file(resource_output, &resource->data_size, 4);
    write_file(resource_output, &header_size, 4);
    write_resource_name(type->ordinal, type->name);
    write_resource_name(resource->ordinal, resource->name);
    if (seek_file(resource_output, 0, SEEK_CUR) & 3)
        write_file(resource_output, &zero, 2);
    write_file(resource_output, &zero, 4);
    write_file(resource_output, &resource->memory_flags, 2);
    write_file(resource_output, &resource->language, 2);
    write_file(resource_output, &resource->version, 4);
    write_file(resource_output, &resource->characteristics, 4);
    resource->data_offset = seek_file(resource_output, 0, SEEK_CUR);
    copy_resource_data(resource_output, data, size, file_bytes);
}

/* Original 0x0100a8d0..0x0100a91b. Preserve the source stream position. */
long __cdecl resource_payload_size(void)
{
    long current, total;
    if (!resource_data_file)
        return resource_data_size;
    current = seek_file(resource_data_file, 0, SEEK_CUR);
    total = resource_data_size + seek_file(resource_data_file, 0, SEEK_END) - current;
    seek_file(resource_data_file, current, SEEK_SET);
    return total;
}

/* Original 0x0100a860..0x0100a8c1. file_bytes=-1 copies/closes the remainder. */
long __stdcall copy_resource_data(FILE *output, void *data, long size, long file_bytes)
{
    if (size > 0)
        write_file(output, data, size);
    if (resource_data_file) {
        if (file_bytes == -1) {
            copy_file_to_end(resource_data_file, output);
            fclose(resource_data_file);
        } else
            copy_file_bytes(resource_data_file, output, file_bytes);
    }
    return size;
}

/* Original 0x01007150..0x010071b1. The old return is the final chunk size;
 * no defined chunk result exists when the requested count is zero. */
unsigned int __stdcall copy_file_bytes(FILE *input, FILE *output, unsigned int count)
{
    BYTE *buffer = resource_alloc(2048);
    unsigned int chunk;
    while (count) {
        chunk = count > 2048 ? 2048 : count;
        count -= chunk;
        read_file(input, buffer, chunk);
        write_file(output, buffer, chunk);
    }
    resource_free(buffer);
    return chunk;
}
/* Original 0x010071c0..0x01007207. Read errors terminate this loop just as EOF
 * does; this function deliberately uses fread rather than checked read_file. */
BOOL __stdcall copy_file_to_end(FILE *input, FILE *output)
{
    BYTE *buffer = resource_alloc(2048);
    unsigned int received;
    while ((received = fread(buffer, 1, 2048, input)) != 0)
        write_file(output, buffer, received);
    resource_free(buffer);
    return TRUE;
}
