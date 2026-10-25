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

/* Search the packed global-name buckets [0040ce70,0040cf38).
 * The input counted length is compared as signed char in the original. */
#pragma intrinsic(memcmp)
int lookup_named_type(unsigned short kind, const unsigned char *name, CachedTypeName **match)
{
    CachedTypeName *cached;
    unsigned hash = name[0], i;
    unsigned char *record;
    unsigned short record_kind;
    for (i = 1; i <= name[0]; ++i) hash += name[i];
    cached = ((CachedTypeName **)type_positions)[hash & 511];
    while (cached) {
        record = cached->record;
        if (!record) return 0;
        if (*(unsigned short *)(record + 2) == kind &&
            (int)cached->name[0] == (signed char)name[0] &&
            !memcmp(cached->name + 1, name + 1, (signed char)name[0])) {
            *match = cached;
            record = cached->record;
            if (record) {
                record_kind = *(unsigned short *)(record + 2);
                if (record_kind >= 0x1004 && record_kind <= 0x1007)
                    return 2 + ((record[6] >> 7) & 1);
            }
            return 2;
        }
        cached = cached->next;
    }
    return 0;
}

/* Search names discovered in this module [0040cda0,0040ce54). */
int find_local_named_type(unsigned short kind, const unsigned char *name, CachedTypeName **match, int scoped)
{
    CachedTypeName *cached;
    unsigned hash = name[0], i;
    unsigned char *record;
    unsigned short record_kind;
    int record_scoped;
    for (i = 1; i <= name[0]; ++i) hash += name[i];
    cached = ((CachedTypeName **)type_references)[hash & 511];
    while (cached) {
        record = cached->record;
        if (!record) return 0;
        if (*(unsigned short *)(record + 2) == kind &&
            (int)cached->name[0] == (signed char)name[0] &&
            !memcmp(cached->name + 1, name + 1, (signed char)name[0])) {
            record_kind = *(unsigned short *)(record + 2);
            record_scoped = record_kind >= 0x1004 && record_kind <= 0x1007 ?
                            (record[6] >> 3) & 1 : 0;
            if (scoped == record_scoped) {
                *match = cached;
                return 1;
            }
        }
        cached = cached->next;
    }
    return 0;
}

/* Encoded CodeView numeric-leaf width [0040fcb0,0040fcfb).
 * Unknown extended tags retain the original tag-minus-0x8000 result. */
unsigned short numeric_leaf_size(const unsigned char *record)
{
    unsigned short leaf = *(const unsigned short *)record;
    switch (leaf) {
    case 0x8000: return 3;  /* LF_CHAR */
    case 0x8001:           /* LF_SHORT */
    case 0x8002: return 4;  /* LF_USHORT */
    case 0x8003:           /* LF_LONG */
    case 0x8004:           /* LF_ULONG */
    case 0x8005: return 6;  /* LF_REAL32 */
    case 0x8006: return 10; /* LF_REAL64 */
    case 0x8007: return 12; /* LF_REAL80 */
    case 0x8008: return 18; /* LF_REAL128 */
    default:
        return leaf < 0x8000 ? 2 : (unsigned short)(leaf - 0x8000);
    }
}

/* Queue a forward-type patch for flush_type_stack [00402e50,00402e94).
 * CachedTypeName's record/reserved fields supply the existing TypePatch view. */
void __fastcall associate_forward_type(unsigned index, CachedTypeName *record)
{
    PendingType *pending = allocate_bytes(sizeof(PendingType));
    pending->index = index;
    pending->patch = (TypePatch *)record;
    pending->next = NULL;
    if (!pending_types_head) pending_types_head = pending_types_tail = pending;
    else {
        pending_types_tail->next = pending;
        pending_types_tail = pending;
    }
}
