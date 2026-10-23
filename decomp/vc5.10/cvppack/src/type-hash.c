/* Type-record hash [0040e1a0,0040e4e1).
 * Each original byte lookup is linear over GF(2). Its eight columns express
 * the map directly; the proof checks all256 values of each original map. */
#include "types.h"
#include "pack.h"
#include <recon.h>

unsigned char type_hash_current, type_hash_previous, type_hash_mix;

unsigned short __fastcall hash_type_record(const unsigned char *data)
{
    static const unsigned char previous_columns[8] = {
        0x01, 0x03, 0x06, 0x0d, 0x1a, 0x34, 0x69, 0xd2
    };
    static const unsigned char current_columns[8] = {
        0xa5, 0x4a, 0x94, 0x28, 0x50, 0xa1, 0x42, 0x85
    };
    static const unsigned char output_columns[8] = {
        0x97, 0x2e, 0x5c, 0xb8, 0x70, 0xe0, 0xc0, 0x80
    };
    const unsigned char *end = data + *(const unsigned short *)data + 2;
    const unsigned char *cursor;
    unsigned short kind, remaining;
    unsigned group, position, bit;
    unsigned char previous, current, transformed, input, output;
    kind = *(const unsigned short *)(data + 2);
    if (kind == 0x1004 || kind == 0x1005 || kind == 0x1006) {
        cursor = data + (kind == 0x1006 ? 12 : 20);
        cursor += *(const unsigned short *)cursor < 0x8000 ? 2 : numeric_leaf_size(cursor);
    } else if (kind == 0x1007) cursor = data + 16;
    else cursor = data;
    type_hash_current = 0;
    type_hash_previous = 0;
    remaining = (unsigned short)(end - cursor);
    while (remaining) {
        /* First consume the partial group, then full groups of eight. Bytes
         * within each group are visited from its last byte to its first. */
        group = ((remaining + 7) & 7) + 1;
        position = group;
        while (position) {
            --position;
            current = type_hash_current;
            previous = type_hash_previous;
            type_hash_previous = current;
            transformed = 0;
            for (bit = 0; bit < 8; ++bit) {
                if (previous & (1U << bit)) transformed ^= previous_columns[bit];
                if (current & (1U << bit)) transformed ^= current_columns[bit];
            }
            type_hash_mix = transformed;
            input = transformed ^ cursor[position];
            output = 0;
            for (bit = 0; bit < 8; ++bit)
                if (input & (1U << bit)) output ^= output_columns[bit];
            type_hash_current = output;
        }
        cursor += group;
        remaining = (unsigned short)(end - cursor);
    }
    return (unsigned short)((((unsigned)type_hash_current << 8) | type_hash_previous) & 0xfff);
}
