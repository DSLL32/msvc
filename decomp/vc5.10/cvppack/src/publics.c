/* Hashing and ownership of packed public-symbol records [0040d1f0,0040d426). */
#include "pack.h"
#include <recon.h>

unsigned public_symbol_bytes, public_symbol_count, duplicate_public_symbols;
int insert_public_symbol(unsigned char *record, SymbolKeyProc key)
{
    unsigned hash, offset = 0, index, length, compare, padding;
    unsigned short name, segment = 0, bucket, name_length;
    unsigned char *text;
    PublicSymbol *node;
    PublicSymbol **buckets = (PublicSymbol **)type_table_global;
    char *conflict;
    padding = 3 - ((record[0] + 1) & 3);
    bucket = key(record, &hash, &name, &segment, &offset, 0x1000);
    text = record + name;
    name_length = text[0];
    for (node = buckets[bucket]; node; node = node->next) {
        if (hash != node->hash) continue;
        for (index = 0; index < (unsigned)name_length + 1; ++index)
            if (text[index] != node->record[node->name_offset + index]) break;
        if (index == (unsigned)name_length + 1) break;
    }
    if (!node) {
        length = *(unsigned short *)record;
        node = allocate_public_block(length + padding + 18);
        node->hash = hash;
        node->name_offset = name;
        node->segment = segment;
        node->offset = offset;
        for (index = 0; index < length + 2; ++index) node->record[index] = record[index];
        *(unsigned short *)node->record += (unsigned short)padding;
        public_symbol_bytes += length + padding + 2;
        ++public_symbol_count;
        for (index = 0; index < padding; ++index) node->record[length + 2 + index] = 0;
        node->next = buckets[bucket];
        buckets[bucket] = node;
        *(unsigned short *)(record + 2) = 8;
        return 1;
    }
    compare = padding - node->name_offset + *(unsigned short *)record - 2;
    for (index = 0; index < compare; ++index)
        if (record[index + 2] != node->record[index + 2]) break;
    if (index == compare) {
        *(unsigned short *)(record + 2) = 8;
        ++duplicate_public_symbols;
        return 0;
    }
    conflict = calloc(1, (unsigned)name_length + 1);
    for (index = 0; index < name_length; ++index) conflict[index] = text[index + 1];
    warning(4002, conflict, module_name(current_module_info));
    free(conflict);
    return 2;
}
