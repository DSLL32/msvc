/* Global symbol records use the public-node arena and their own hash heads.
 * [0040d430,0040d5f2). */
#include "pack.h"
#include <recon.h>

unsigned global_symbol_bytes, global_symbol_count;
extern unsigned duplicate_public_symbols;

int __fastcall insert_global_symbol(unsigned char *record, SymbolKeyProc key)
{
    unsigned hash, offset = 0, index, length, padding;
    unsigned short name, segment = 0, bucket, name_length;
    unsigned char *text;
    PublicSymbol *node;
    PublicSymbol **buckets = (PublicSymbol **)type_table_local;
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
        padding = 3 - ((record[0] + 1) & 3);
        node = allocate_public_block(length + padding + 18);
        node->hash = hash;
        node->name_offset = name;
        node->segment = segment;
        node->offset = offset;
        for (index = 0; index < length + 2; ++index) node->record[index] = record[index];
        *(unsigned short *)node->record += (unsigned short)padding;
        global_symbol_bytes += length + padding + 2;
        ++global_symbol_count;
        for (index = 0; index < padding; ++index) node->record[length + 2 + index] = 0;
        node->next = buckets[bucket];
        buckets[bucket] = node;
        *(unsigned short *)(record + 2) = 8;
        return 1;
    }
    length = *(unsigned short *)record;
    for (index = 0; index < length; ++index)
        if (record[index + 2] != node->record[index + 2]) break;
    if (index == length) {
        *(unsigned short *)(record + 2) = 8;
        ++duplicate_public_symbols;
        return 0;
    }
    return 2;
}
