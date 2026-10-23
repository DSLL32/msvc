/* Retain references to module-local symbol records [0040f4a0,0040f5c1). */
#include "types.h"
#include "pack.h"
#include <recon.h>

extern unsigned global_symbol_bytes, global_symbol_count;
unsigned reference_symbol_bytes, reference_symbol_count;

void record_symbol_references(unsigned char *buffer, unsigned size, unsigned short module)
{
    unsigned char *record = buffer + 4;
    unsigned offset = 4;
    unsigned short kind;
    while (offset < size) {
        kind = *(unsigned short *)(record + 2);
        switch (kind) {
        case 0x105: case 0x100b: case 0x1011:
            record_symbol_reference(record, (PublicSymbol **)type_table_local,
                                    module, offset, 0x400);
            global_symbol_bytes += 16;
            ++global_symbol_count;
            offset = *(unsigned *)(record + 8);
            record = buffer + offset;
            break;
        case 0x104: case 0x100a: case 0x1010:
            record_symbol_reference(record, (PublicSymbol **)type_table_forward,
                                    module, offset, 0x400);
            offset = *(unsigned *)(record + 8);
            record = buffer + offset;
            reference_symbol_bytes += 16;
            ++reference_symbol_count;
            break;
        case 0x101: case 0x102: case 0x1007: case 0x1008:
        case 0x100e: case 0x100f:
            record_symbol_reference(record, (PublicSymbol **)type_table_forward,
                                    module, offset, 0x401);
            reference_symbol_bytes += 16;
            ++reference_symbol_count;
            break;
        }
        offset += *(unsigned short *)record + 2;
        record += *(unsigned short *)record + 2;
    }
}
