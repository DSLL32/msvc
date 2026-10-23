/* Locate an equivalent recursive type [0040cc10,0040cca4). */
#include "types.h"
#include "pack.h"
#include <recon.h>

unsigned resolve_recursive_type(TypeRecord *record, unsigned index)
{
    unsigned bucket = hash_recursive_type(record);
    RecursiveTypeNode *entry = ((RecursiveTypeNode **)type_table_primary)[bucket];
    int match;
    for (; entry; entry = entry->next) {
        if (entry->index < module_type_base) match = 0;
        else if (record->status & 1) match = record->mapped_index == entry->index;
        else if (record->status & 2) match = 1;
        else if (record->status & 4) match = 1;
        else match = compare_recursive_type(record, index, entry->data, entry->index);
        if (match) return entry->index;
    }
    return intern_recursive_type(index);
}

/* Allocate and recursively resolve an unmatched type [00408900,00408b35). */
unsigned __fastcall intern_recursive_type(unsigned index)
{
    unsigned slot = index - module_type_base;
    TypeRecord *record;
    CachedTypeName *match;
    unsigned first_global, second_global;
    unsigned char *data;
    unsigned short i, offset, kind;
    unsigned mask, flags;
    int found;
    if (!module_type_low && slot == 0xffff - module_type_base) record = empty_type_record;
    else {
        record = &type_records[slot];
        if (record->status & 0x1000)
            record = lookup_global_type(record->global_index - 0x1000, &first_global);
    }
    if (record->status & 0x1000) {
        slot = record->global_index - module_type_base;
        if (!module_type_low && slot == 0xffff - module_type_base) record = empty_type_record;
        else {
            record = &type_records[slot];
            if (record->status & 0x1000)
                record = lookup_global_type(record->global_index - 0x1000, &second_global);
        }
    }
    if (!(record->status & (0x6 | 0x800))) {
        found = match_forward_type(record, &match, 0);
        if (found == 3) {
            data = record->data;
            record->mapped_index = match->type_index;
            record->data = match->record;
        } else {
            allocate_global_type(record);
            data = record->data;
        }
        record->status |= 4;
        mark_recursive_type(record);
        for (i = 0; i < record->reference_count; ++i) {
            mask = 0x80 >> (record->flags & 1 ? i & 7 : i);
            flags = record->flags & 1 ? record->reference_flags[i >> 3] :
                record->reference_offsets[8];
            if (flags & mask) continue;
            offset = record->flags & 1 ? record->extended_references[i] :
                record->reference_offsets[i];
            *(unsigned *)(data + offset) = intern_recursive_type(*(unsigned *)(data + offset));
            if (record->flags & 1)
                record->reference_flags[i >> 3] |= (unsigned char)(0x80 >> (i & 7));
            else record->reference_offsets[8] |= (unsigned char)(0x80 >> i);
        }
        if (found == 3) memmove(match->record, data, *(unsigned short *)data + 2);
        index_recursive_type(record->data, record->mapped_index);
        kind = *(unsigned short *)(record->data + 2);
        if (kind == 0x1004 || kind == 0x1005)
            record_base_classes(record->mapped_index, *(unsigned *)(record->data + 8));
    }
    return record->mapped_index;
}

/* Remember a recursive type before following its references [0040c080,0040c0f8). */
unsigned recursive_nodes_remaining;
RecursiveTypeNode *recursive_node_cursor;

void __fastcall mark_recursive_type(TypeRecord *record)
{
    unsigned bucket;
    RecursiveTypeNode *node;
    if (!recursive_nodes_remaining) {
        recursive_node_cursor = allocate_bytes(0x3000);
        recursive_nodes_remaining = 1024;
    }
    bucket = hash_recursive_type(record);
    --recursive_nodes_remaining;
    node = recursive_node_cursor++;
    node->data = record->data;
    node->index = record->mapped_index;
    node->next = ((RecursiveTypeNode **)type_table_primary)[bucket];
    ((RecursiveTypeNode **)type_table_primary)[bucket] = node;
    ++((unsigned short *)type_table_primary_counts)[bucket];
}
