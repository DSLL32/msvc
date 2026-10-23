/* Locate an equivalent recursive type [0040cc10,0040cca4). */
#include "types.h"
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
