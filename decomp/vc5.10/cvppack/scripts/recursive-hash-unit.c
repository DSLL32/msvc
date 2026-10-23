/* Native-only hash/storage boundaries, not a replacement original oracle. */
#define RECON_SHIM_IMPL
#include "types.h"
#include <recon.h>
#undef main

int main(void)
{
    TypeRecord record;
    unsigned char data[48], flags = 0xe0;
    unsigned short offsets[6] = { 4, 12, 20, 28, 36, 44 };
    unsigned i, named, generic, extended, cached;
    memset(&record, 0, sizeof(record));
    memset(data, 0, sizeof(data));
    *(unsigned short *)data = 27;
    *(unsigned short *)(data + 2) = 0x1005;
    data[22] = 6;
    memcpy(data + 23, "Widget", 6);
    record.data = data;
    named = hash_recursive_type(&record);
    memset(&record, 0, sizeof(record));
    for (i = 0; i < sizeof(data); ++i) data[i] = (unsigned char)i;
    *(unsigned short *)data = 14;
    *(unsigned short *)(data + 2) = 0x1001;
    record.data = data;
    record.reference_count = 1;
    record.reference_offsets[0] = 8;
    record.reference_offsets[8] = 0x80;
    generic = hash_recursive_type(&record);
    memset(&record, 0, sizeof(record));
    *(unsigned short *)data = 46;
    record.data = data;
    record.reference_count = 6;
    record.extended_references = offsets;
    record.reference_flags = &flags;
    record.flags = 1;
    extended = hash_recursive_type(&record);
    record.data = NULL;
    record.recursive_hash = 0xabc;
    cached = hash_recursive_type(&record);
    printf("%u %u %u %u\n", named, generic, extended, cached);
    return 0;
}
