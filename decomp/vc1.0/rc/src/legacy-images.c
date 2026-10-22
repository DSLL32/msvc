/* Old 2.03 image resources have either a retained twelve-byte description or
 * an external image table preceding their remaining bytes. */
#include "resource-data.h"

/* Original 0x0100a920..0x0100a9e3. */
unsigned int __stdcall read_icon_file_header(unsigned int size)
{
    struct {
        BYTE description[6];
        short columns, rows;
        WORD reserved;
    } header;
    union {
        WORD value;
        struct { BYTE format, inline_header; } parts;
    } tag;
    FILE *file = resource_stream(0);
    long table_end;
    BOOL retained = FALSE;
    read_file(file, &tag, 2);
    if (tag.parts.format != 1 && tag.parts.format != 3)
        resource_data_error("Resource file %s is not in 2.03 format.", current_token.spelling);
    read_file(file, &header, sizeof(header));
    table_end = header.columns * header.rows * 2 + 14;
    if ((long)size > table_end) {
        tag.parts.inline_header = 0;
        seek_file(file, table_end, SEEK_SET);
        put_word(tag.value);
    } else {
        tag.parts.inline_header = 1;
        put_word(tag.value);
        put_bytes(&header, sizeof(header));
        retained = TRUE;
    }
    return size - seek_file(file, 0, SEEK_CUR) + (retained ? 14 : 2);
}
