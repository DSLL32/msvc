/* Intern a completed type by record bytes [0040b210,0040b3af). */
#include "types.h"
#include "pack.h"
#include <recon.h>

#pragma intrinsic(memcmp)

void __fastcall intern_type(TypeRecord *record)
{
    InternedType **buckets = (InternedType **)type_table_secondary;
    InternedType *entry;
    CachedTypeName *match;
    unsigned short bucket = hash_type_record(record->data);
    unsigned char *data, *name;
    unsigned short kind;
    for (entry = buckets[bucket]; entry; entry = entry->next) {
        if (entry->data && !memcmp(entry->data, record->data, *(unsigned short *)entry->data + 2)) {
            record->status |= 2;
            record->mapped_index = entry->index;
            if (record->status & 0x38) release_type_payload(record);
            if (record->status & 0x100) {
                free(record->extended_references);
                free(record->reference_flags);
                record->status &= 0xfeff;
            }
            return;
        }
    }
    data = record->data;
    kind = *(unsigned short *)(data + 2);
    if (kind == 0x1004 || kind == 0x1005 || kind == 0x1006) {
        name = data + (kind == 0x1006 ? 12 : 20);
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
    } else if (kind == 0x1007) name = data + 16;
    else name = NULL;
    if (name && lookup_named_type(kind, name, &match) == 3) {
        record->mapped_index = match->type_index;
        memmove(match->record, data, *(unsigned short *)data + 2);
        record->data = match->record;
    } else allocate_global_type(record);
    entry = allocate_bytes(sizeof(InternedType));
    entry->data = record->data;
    entry->index = record->mapped_index;
    buckets = (InternedType **)type_table_secondary;
    entry->next = buckets[bucket];
    buckets[bucket] = entry;
    ++((unsigned short *)type_table_secondary_counts)[bucket];
    data = entry->data;
    kind = *(unsigned short *)(data + 2);
    if ((kind == 0x1004 || kind == 0x1005) && !(data[6] & 0x80))
        record_base_classes(entry->index, *(unsigned *)(data + 8));
}
