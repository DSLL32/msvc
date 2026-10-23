/* Compact pointer referents and embedded based-pointer symbols
 * [00403a90,00403bdb). */
#include "types.h"
#include "pack.h"
#include <recon.h>

unsigned char __fastcall compact_legacy_pointer(TypeRecord *record)
{
    TypeIndexMapper map = type_index_mappers[module_type_low];
    unsigned char *data;
    unsigned attributes, mode, pointer_kind, result;
    unsigned short symbol_kind, offset;
    result = map(*(unsigned *)(record->data + 4));
    *(unsigned *)(record->data + 4) = result;
    data = record->data + 2;
    if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 4;
    attributes = *(unsigned *)(data + 6);
    mode = (attributes >> 5) & 7;
    if (mode == 2 || mode == 3) {
        result = map(*(unsigned *)(data + 10));
        *(unsigned *)(record->data + 12) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 12;
    } else if (!mode) {
        pointer_kind = attributes & 31;
        if (pointer_kind >= 4 && pointer_kind <= 7) {
            symbol_kind = *(unsigned short *)(data + 12);
            switch (symbol_kind) {
            case 0x0100: offset = 18; break;
            case 0x0101: case 0x0102: case 0x1006: offset = 20; break;
            case 0x1001: case 0x1007: case 0x1008:
            case 0x100e: case 0x100f: offset = 16; break;
            default:
                /* The original local is unwritten here. At00403b76 it reads
                 * the stack slot reserved by PUSH ECX, retaining incoming
                 * pointer bits. No unsupported-kind consumer is validated. */
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
                __builtin_debugtrap();
#endif
                break;
            }
            result = map(*(unsigned *)(record->data + offset));
            *(unsigned *)(record->data + offset) = result;
            if (recursive_type_pending)
                record->reference_offsets[record->reference_count++] = (unsigned char)offset;
        } else if (pointer_kind == 8) {
            result = map(*(unsigned *)(data + 10));
            *(unsigned *)(record->data + 12) = result;
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 12;
        }
    }
    return (unsigned char)record->reference_count;
}
