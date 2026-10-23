/* Native-only reference storage boundaries; not a new original oracle. */
#define RECON_SHIM_IMPL
#include "types.h"
#include <recon.h>
#undef main

int main(void)
{
    TypeRecord record;
    unsigned i;
    memset(&record, 0, sizeof(record));
    record.reference_offsets[8] = 0xa5;
    for (i = 0; i < 5; ++i) add_type_reference(&record, (unsigned short)(i + 1));
    if (record.reference_count != 5 || record.flags & 1) return 120;
    add_type_reference(&record, 260);
    if (record.reference_count != 6 || !(record.flags & 1) ||
        record.reference_flags[0] != 0xa5 || record.extended_references[5] != 260) return 121;
    for (i = 0; i < 5; ++i) if (record.extended_references[i] != i + 1) return 122;
    for (i = 6; i < 40; ++i) add_type_reference(&record, (unsigned short)(i + 256));
    record.reference_flags[0] = 0xa5;
    add_type_reference(&record, 296);
    if (record.reference_count != 41 || record.reference_flags[0]) return 123;
    for (i = 6; i <= 40; ++i) if (record.extended_references[i] != i + 256) return 124;
    memset(&record, 0, sizeof(record));
    add_type_reference(&record, 256);
    if (record.reference_count != 1 || !(record.flags & 1) ||
        record.extended_references[0] != 256) return 125;
    return 0;
}
