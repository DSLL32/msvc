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

/* Locate the Pascal name and produce its case-folded hash [0040cf50,0040d0f9).
 * Output fields retain their individual original WORD/DWORD widths. */
unsigned short symbol_record_key(const unsigned char *record, unsigned *hash,
    unsigned short *name, unsigned short *segment, unsigned *offset, unsigned flags)
{
    unsigned short kind = *(const unsigned short *)(record + 2), section = 0;
    unsigned address = 0, folded = 0, tail = 0, value, length, count;
    const unsigned char *text;
    if (kind >= 0x101 && kind <= 0x103) {
        section = *(const unsigned short *)(record + 6);
        address = *(const unsigned short *)(record + 4);
        text = record + 10;
    } else if (kind == 0x104 || kind == 0x105) {
        section = *(const unsigned short *)(record + 24);
        address = *(const unsigned short *)(record + 22);
        text = record + 29;
    } else if (kind == 0x1002) {
        text = record + 8;
        text += *(const unsigned short *)text < 0x8000 ? 2 : numeric_leaf_size(text);
    } else if (kind == 0x1003) {
        text = record + 8;
    } else if ((kind >= 0x1007 && kind <= 0x1009) || kind == 0x100e || kind == 0x100f) {
        section = *(const unsigned short *)(record + 12);
        address = *(const unsigned *)(record + 8);
        text = record + 14;
    } else if (kind == 0x100a || kind == 0x100b) {
        section = *(const unsigned short *)(record + 36);
        address = *(const unsigned *)(record + 32);
        text = record + 39;
    } else if (kind == 0x1010 || kind == 0x1011) {
        section = *(const unsigned short *)(record + 52);
        address = *(const unsigned *)(record + 48);
        text = record + 56;
    } else {
        *segment = 0;
        *offset = *hash = 0;
        return 0;
    }
    *name = (unsigned short)(text - record);
    length = *text++;
    while (length & 3) {
        tail = (tail | (text[length - 1] & 0xdf)) << 8;
        --length;
    }
    count = length / 4;
    while (count--) {
        value = (*(const unsigned *)text & 0xdfdfdfdf) ^ folded;
        folded = (value << 4) | (value >> 28);
        text += 4;
    }
    folded ^= tail;
    *segment = section;
    *offset = address;
    *hash = folded;
    return (unsigned short)(folded % (unsigned short)flags);
}

/* Original bump allocation for public-symbol nodes [0040b1d0,0040b20d). */
unsigned public_arena_remaining;
unsigned char *public_arena_cursor;
PublicSymbol *allocate_public_block(unsigned size)
{
    unsigned char *result;
    unsigned remaining = public_arena_remaining;
    if (remaining < size) {
        result = allocate_bytes(0x1fa4);
        remaining = 0x1fa4;
    } else result = public_arena_cursor;
    public_arena_cursor = result + size;
    public_arena_remaining = remaining - size;
    return (PublicSymbol *)result;
}
