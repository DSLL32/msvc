/* Finish derived-class lists and patch their owning class records
 * [0040df60,0040e1a0). */
#include "types.h"
#include "pack.h"
#include "widen.h"
#include <recon.h>

#pragma intrinsic(memcmp)

typedef struct DerivedTypeEntry {
    unsigned base_index;
    unsigned short count, capacity;
    unsigned *indices;
} DerivedTypeEntry;
typedef struct DerivedTypeList {
    unsigned short length, kind;
    unsigned count;
    unsigned indices[1];
} DerivedTypeList;
unsigned short derived_type_count;

void finalize_derived_types(void)
{
    unsigned short ordinal, item, hash;
    unsigned index;
    unsigned char *base;
    DerivedTypeEntry *entry;
    DerivedTypeList *list;
    InternedType *node;
    char limit[12];
    if (discard_types) return;
    for (ordinal = 0; ordinal < derived_type_count; ++ordinal) {
        entry = (DerivedTypeEntry *)type_hash_buckets + ordinal;
        list = (DerivedTypeList *)allocate_type_buffer(entry->count * 4 + 8);
        list->length = (unsigned short)(entry->count * 4 + 6);
        list->kind = 0x1204;
        list->count = entry->count;
        for (item = 0; item < entry->count; ++item)
            list->indices[item] = entry->indices[entry->count - item - 1];
        free(entry->indices);
        hash = hash_type_record((unsigned char *)list);
        for (node = ((InternedType **)type_table_secondary)[hash]; node; node = node->next)
            if (!memcmp(node->data, list, *(unsigned short *)node->data + 2)) break;
        index = entry->base_index;
        base = global_type_blocks[(index - 0x1000) >> 9][index & 511];
        if (node) *(unsigned *)(base + 12) = node->index;
        else {
            node = allocate_bytes(sizeof(InternedType));
            node->index = symbol_rewriters.state.next_global_type;
            node->data = copy_type_arena(&type_large_arena, (unsigned char *)list, list->length + 2);
            node->next = ((InternedType **)type_table_secondary)[hash];
            ((InternedType **)type_table_secondary)[hash] = node;
            ++((unsigned short *)type_table_secondary_counts)[hash];
            index = symbol_rewriters.state.next_global_type;
            if (index - 0x1000 == type_table_used) {
                type_table_used += 512;
                global_type_blocks[(index - 0x1000) >> 9] = allocate_bytes(512 * sizeof(void *));
                index = symbol_rewriters.state.next_global_type;
            }
            global_type_blocks[(index - 0x1000) >> 9][index & 511] = node->data;
            *(unsigned *)(base + 12) = symbol_rewriters.state.next_global_type;
            ++symbol_rewriters.state.next_global_type;
            if (!symbol_rewriters.state.next_global_type || symbol_rewriters.state.next_global_type > 0xffffff) {
                sprintf(limit, "%lu", 0xfff000UL);
                warning(4009, limit, module_name(current_module_info));
                discard_types = 1;
                return;
            }
        }
    }
}

/* Accumulate derived indices for each base class [0040e520,0040e63f). */
void record_base_relation(unsigned index, unsigned base)
{
    unsigned short ordinal;
    DerivedTypeEntry *entry;
    for (ordinal = 0; ordinal < derived_type_count; ++ordinal)
        if (((DerivedTypeEntry *)type_hash_buckets)[ordinal].base_index == base) break;
    if (ordinal == derived_type_count) {
        if (derived_type_count == derived_type_capacity) {
            derived_type_capacity += 128;
            type_hash_buckets = resize_bytes(type_hash_buckets,
                (unsigned)derived_type_capacity * sizeof(DerivedTypeEntry));
        }
        entry = (DerivedTypeEntry *)type_hash_buckets + ordinal;
        entry->count = 0;
        entry->capacity = 20;
        entry->base_index = base;
        entry->indices = allocate_bytes(80);
        ++derived_type_count;
    }
    entry = (DerivedTypeEntry *)type_hash_buckets + ordinal;
    if (entry->count == entry->capacity) {
        entry->capacity += 20;
        entry->indices = resize_bytes(entry->indices, (unsigned)entry->capacity * 4);
    }
    entry->indices[entry->count++] = index;
}
