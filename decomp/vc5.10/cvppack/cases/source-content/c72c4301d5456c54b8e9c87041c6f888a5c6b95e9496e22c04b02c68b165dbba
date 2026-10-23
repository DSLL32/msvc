/* Recursive type-reference offsets [00404360,004044cf). */
#include "types.h"
#include <recon.h>

void __fastcall add_type_reference(TypeRecord *record, unsigned short offset)
{
    unsigned short i, count = record->reference_count;
    unsigned short *offsets;
    unsigned char *flags;
    if (offset <= 255 && count < 5) {
        record->reference_offsets[count] = (unsigned char)offset;
        ++record->reference_count;
        return;
    }
    if (!(record->flags & 1)) {
        offsets = (unsigned short *)allocate_cleared(20);
        flags = allocate_cleared(40);
        for (i = 0; i < count; ++i) offsets[i] = record->reference_offsets[i];
        flags[0] = record->reference_offsets[8];
        record->extended_references = offsets;
        record->reference_flags = flags;
    } else if (!(count % 10)) {
        offsets = (unsigned short *)allocate_cleared(count * 2 + 20);
        for (i = 0; i < count; ++i) offsets[i] = record->extended_references[i];
        free(record->extended_references);
        record->extended_references = offsets;
        if (!(count & 7)) {
            flags = allocate_cleared(count + 10);
            /* Retain the original remainder-based copy bound. At this
             * aligned growth point it copies no prior flag bytes. */
            for (i = 0; i < (count & 7); ++i) flags[i] = record->reference_flags[i];
            free(record->reference_flags);
            record->reference_flags = flags;
        }
    }
    record->extended_references[count] = offset;
    record->flags |= 1;
    ++record->reference_count;
}
