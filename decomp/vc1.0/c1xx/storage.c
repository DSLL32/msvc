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

/* Original 0x61d80..0x61daa. Only the three extended storage bits take
   part in this comparison; kind and ordinary flags are checked by callers. */
int __stdcall equivalent_storage_extensions(StorageSpec left, StorageSpec right)
{
    return !((left.extended ^ right.extended) & 7);
}

/* Original 0x61db0..0x61e5e. Inherit selected ordinary modifiers and the
   first extended bit. The DLL export bit can be added only when both
   declarations already carry the accompanying DLL storage bit. Unlike
   merge_storage_spec, this helper reports whether the destination changed. */
int __stdcall inherit_storage_modifiers(StorageSpec *destination, StorageSpec source)
{
    unsigned short additions = (source.flags & 0x1340) & ~destination->flags;
    int changed = 0;
    if (additions) {
        destination->flags |= additions;
        changed = 1;
    }
    if ((source.extended & 1) && !(destination->extended & 1)) {
        destination->extended |= 1;
        changed = 1;
    }
    if ((source.extended & 8) && (destination->extended & 4) &&
        (source.extended & 4) && !(destination->extended & 8)) {
        destination->extended |= 8;
        changed = 1;
    }
    return changed;
}
