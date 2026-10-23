/* Allocate packed global types and their counted-name cache [0040b490,0040b878). */
#include "types.h"
#include "pack.h"
#include <recon.h>

#pragma intrinsic(memcpy, memcmp)

unsigned next_global_type = 0x1000;
int global_types_exhausted;
unsigned short retain_structure_names;
unsigned char **global_type_blocks[2048];
StructureAlias *structure_aliases, *last_structure_alias;

void __fastcall allocate_global_type(TypeRecord *record)
{
    unsigned char *data, *name, **block;
    unsigned index, hash, length, padding;
    unsigned short bytes, kind;
    CachedTypeName *cached, **buckets;
    StructureAlias *alias;
    char limit[12];
    if (global_types_exhausted) {
        record->mapped_index = 0;
        fflush(stdout);
        return;
    }
    data = record->data;
    if (*(unsigned short *)(data + 2) == 0x1005 && retain_structure_names) {
        name = data + 20;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        if (*name && !(*name == 10 && !memcmp(name + 1, "(untagged)", 10))) {
            length = *name + 9;
            padding = 3 - ((length - 1) & 3);
            bytes = (unsigned short)(padding + length - 2);
            alias = allocate_bytes(bytes + 6);
            if (!structure_aliases) structure_aliases = alias;
            else last_structure_alias->next = alias;
            alias->length = bytes;
            alias->kind = 0x1003;
            alias->index = next_global_type;
            memcpy(alias->name, name, *name + 1);
            for (length = 0; length < padding; ++length) alias->name[*name + 1 + length] = 0;
            last_structure_alias = alias;
            alias->next = NULL;
        }
    }
    if (next_global_type - 0x1000 == type_table_used) {
        type_table_used += 512;
        global_type_blocks[(next_global_type - 0x1000) >> 9] = allocate_bytes(512 * sizeof(void *));
    }
    bytes = (unsigned short)(*(unsigned short *)data + 2);
    if (bytes > 0xc000) {
        *(unsigned short *)data = 2;
        *(unsigned short *)(data + 2) = 0x000f;
        bytes = 4;
        warning(4006, NULL, NULL);
    }
    block = global_type_blocks[(next_global_type - 0x1000) >> 9];
    block[next_global_type & 511] = copy_type_arena(&type_large_arena, data, bytes);
    if (record->status & 0x38) release_type_payload(record);
    record->data = block[next_global_type & 511];
    record->mapped_index = next_global_type;
    index = next_global_type++;
    data = record->data;
    kind = *(unsigned short *)(data + 2);
    if (kind == 0x1004 || kind == 0x1005 || kind == 0x1006) {
        name = data + (kind == 0x1006 ? 12 : 20);
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
    } else if (kind == 0x1007) name = data + 16;
    else name = NULL;
    if (name) {
        hash = *name;
        for (length = 1; length <= *name; ++length) hash += name[length];
        buckets = (CachedTypeName **)type_positions;
        hash &= 511;
        for (cached = buckets[hash]; cached; cached = cached->next) {
            if (!cached->record) break;
            if (*(unsigned short *)(cached->record + 2) == kind && *cached->name == *name &&
                !memcmp(cached->name + 1, name + 1, *name)) goto checked_limit;
        }
        if (!cached) {
            cached = allocate_cleared(sizeof(CachedTypeName));
            if (!cached) fatal_error(1001, NULL, NULL);
            cached->next = buckets[hash];
            buckets[hash] = cached;
        }
        cached->record = data;
        cached->name = name;
        cached->type_index = index;
    }
checked_limit:
    if (!next_global_type || next_global_type > 0xffffff) {
        sprintf(limit, "%lu", 0xfff000UL);
        warning(4009, limit, module_name(current_module_info));
        global_types_exhausted = 1;
    }
}
