#include "symbol-storage.h"
#include <recon.h>

/* These are distinct original DWORDs, not the section alignment selected by
 * initialize_machine_registers. */
int local_storage_alignment = 4;
int argument_stack_offset = 8;
int argument_layout_kind = 4;
int argument_alignment = 4;

unsigned __fastcall reserve_symbol_storage(C2Symbol *symbol)
{
    C2InputSymbol *variable;
    unsigned attributes, storage, subtype;
    long bytes, offset, remainder;

    if (symbol->kind != 1 || (symbol->flags & 0x800))
        return 0;
    variable = (C2InputSymbol *)symbol;
    attributes = variable->attributes;
    bytes = symbol->value;
    storage = (attributes >> 4) & 7;
    subtype = (attributes >> 7) & 15;
    switch (storage) {
    case 1:
        symbol->parameter_offset = allocate_local_storage(bytes, local_storage_alignment);
        break;
    case 2: case 4:
        switch (subtype) {
        case 1: case 4:
            reserve_section_symbol(symbol);
            break;
        case 2:
            break;
        case 3:
            if (symbol->flags & 0x200) {
                variable->attributes = (attributes & ~0x700U) | 0x80;
                reserve_section_symbol(symbol);
            } else
                variable->attributes = (attributes & ~0x50U) | 0x20;
            break;
        default:
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x39f);
            break;
        }
        break;
    case 3:
        offset = argument_stack_offset;
        if (offset == 8 && ((current_symbol->flags & 2) ||
            (!(current_symbol->flags & 1) && argument_layout_kind == 6)))
            offset = 12;
        remainder = bytes % argument_alignment;
        symbol->parameter_offset = offset;
        if (remainder)
            bytes += argument_alignment - remainder;
        argument_stack_offset = offset + bytes;
        break;
    case 5:
        if (symbol->flags & 0x20)
            reserve_section_symbol(symbol);
        break;
    case 6:
        if (subtype == 1 || subtype == 4)
            reserve_section_symbol(symbol);
        else
            c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x37e);
        break;
    case 7:
        reserve_section_symbol(symbol);
        break;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\x86\\MDmisc.c", 0x3b7);
        break;
    }
    symbol->flags |= 0x800;
    return bytes;
}
