/* Declaration storage modifiers shared by parser and declaration binding. */
#include "declarations.h"

/* Original 0x61c90..0x61d73. Storage kind and the unlisted modifier bits are
   excluded. The result reports selected source bits, even if already present
   in the destination; it does not report whether the destination changed. */
int __stdcall merge_storage_spec(StorageSpec *destination, StorageSpec source)
{
    unsigned short flags = source.flags & 0x37f8;
    unsigned char extended = source.extended & 15;
    if (flags) destination->flags |= flags;
    if (extended) destination->extended |= extended;
    return flags != 0 || extended != 0;
}
