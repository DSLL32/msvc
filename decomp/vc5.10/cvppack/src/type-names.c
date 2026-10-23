/* Select a class/union/enum counted name and search the two caches
 * [0040ccb0,0040cd78). */
#include "types.h"
#include "pack.h"
#include <recon.h>

int __fastcall match_forward_type(TypeRecord *record, CachedTypeName **match, int insert)
{
    const unsigned char *data = record->data, *name;
    unsigned short kind = *(const unsigned short *)(data + 2);
    int scoped, result;
    if (kind == 0x1004 || kind == 0x1005 || kind == 0x1006) {
        name = data + (kind == 0x1006 ? 12 : 20);
        name += *(const unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
    } else if (kind == 0x1007) name = data + 16;
    else return 0;
    if (insert) {
        scoped = (data[6] >> 3) & 1;
        result = find_local_named_type(kind, name, match, scoped);
        if (result == 1) return result;
    }
    return lookup_named_type(kind, name, match);
}
