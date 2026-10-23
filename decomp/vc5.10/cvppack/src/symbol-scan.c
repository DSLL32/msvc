/* First pass over modern module symbols [0040a380,0040a714). */
#include "types.h"
#include "pack.h"
#include <recon.h>

#pragma intrinsic(memcpy, memcmp)

unsigned short symbol_segment_count;
unsigned short symbol_segments[255];

/* Test the current module's segment list [0040def0,0040df2b). */
int has_symbol_segment(unsigned short segment)
{
    unsigned short i;
    for (i = 0; i < symbol_segment_count; ++i)
        if (symbol_segments[i] == segment) return 1;
    return 0;
}

/* Reserve a segment-change record once per segment [0040a730,0040a78c). */
void add_symbol_segment(unsigned short segment, unsigned short *symbols)
{
    if (!has_symbol_segment(segment)) {
        *symbols += 13;
        symbol_segments[symbol_segment_count++] = segment;
        if (symbol_segment_count > 254)
            fatal_error(1015, module_name(current_module_info), NULL);
    }
}

void scan_module_symbols(void *lines, unsigned size, unsigned short *symbols,
                         unsigned short *references)
{
    unsigned char *first = (unsigned char *)lines + 4;
    unsigned char *end = (unsigned char *)lines + size;
    unsigned char *record, *name, *type;
    unsigned from_global;
    unsigned short old_symbols, old_references, length, kind;
    unsigned index, slot, hash, count, i;
    TypeRecord *entry;
    CachedTypeName **buckets = (CachedTypeName **)type_references, *cached;

restart:
    old_symbols = *symbols;
    old_references = *references;
    symbol_segment_count = 0;
    for (record = first; record < end; record += length) {
        /* Some records start at odd addresses; all fields are little endian. */
        length = (unsigned short)(record[0] | (record[1] << 8)) + 2;
        kind = record[2] | (record[3] << 8);
        if (kind == 7) {
            *references += length;
            continue;
        }
        *symbols += 3 - ((length - 1) & 3);
        if (kind == 0x104 || kind == 0x105) {
            add_symbol_segment(*(unsigned short *)(record + 24), symbols);
        } else if (kind == 0x100a || kind == 0x100b) {
            add_symbol_segment(*(unsigned short *)(record + 36), symbols);
        } else if (kind == 0x1010 || kind == 0x1011) {
            add_symbol_segment(*(unsigned short *)(record + 52), symbols);
        } else if (kind == 0x1003) {
            index = *(unsigned *)(record + 4);
            name = record + 8;
            if (index >= 0x1000) {
                slot = index - module_type_base;
                if (!(type_records[slot].status & 0x4000)) {
                    if (!module_type_low && slot == 0xffff - module_type_base) entry = empty_type_record;
                    else {
                        entry = &type_records[slot];
                        if (entry->status & 0x1000)
                            entry = lookup_global_type(entry->global_index - 0x1000, &from_global);
                    }
                    type = entry->data;
                    if (type && (*(unsigned short *)(type + 2) == 0x1004 ||
                                 *(unsigned short *)(type + 2) == 0x1005 ||
                                 *(unsigned short *)(type + 2) == 0x1006 ||
                                 *(unsigned short *)(type + 2) == 0x1007)) {
                        count = name[0];
                        hash = count;
                        for (i = 0; i < count; ++i) hash += name[i + 1];
                        hash &= 511;
                        cached = buckets[hash];
                        while (cached && cached->record) {
                            if (cached->name[0] == count && !memcmp(cached->name + 1, name + 1, count)) {
                                if (cached->type_index != index) module_type_list_used |= module_type_entries;
                                break;
                            }
                            cached = cached->next;
                        }
                        if (!cached) {
                            cached = allocate_cleared(sizeof(CachedTypeName));
                            if (!cached) fatal_error(1001, NULL, NULL);
                            cached->next = buckets[hash];
                            buckets[hash] = cached;
                        }
                        cached->record = type;
                        cached->name = allocate_bytes(name[0] + 1);
                        memcpy(cached->name, name, name[0] + 1);
                        cached->type_index = index;
                    }
                }
            }
            if (module_type_list_used) {
                module_type_entries = module_type_list_used = 0;
                *symbols = old_symbols;
                *references = old_references;
                initialize_module_type_storage();
                reset_type_name_cache();
                read_type_database(NULL);
                if (!module_precompiled_limit) module_precompiled_limit = type_record_count;
                goto restart;
            }
        }
    }
}
