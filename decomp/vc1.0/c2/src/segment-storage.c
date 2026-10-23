/* Default storage segments and per-symbol COMDAT segments. */
#include "records.h"
#include "segments.h"
#include "function-input.h"
#include "frame.h"
#include <recon.h>
extern int default_segment_alignment, stack_alignment;
extern int code_pointer_bytes, default_integer_bytes;
void __fastcall reserve_segment_storage(SymbolHeader *symbol);
/* Original [00027080,00027200), including its six-way storage table.
   Mark each variable once and return its contribution to parameter/local size. */
unsigned __fastcall reserve_symbol_storage(SymbolHeader *header)
{
    TemporarySymbol *symbol = (TemporarySymbol *)header;
    unsigned bytes, storage, allocation, offset;
    unsigned short flags;
    int remainder;
    if (header->kind != 1) return 0;
    flags = header->field0e;
    if (flags & 0x800) return 0;
    bytes = header->value;
    storage = (symbol->attributes & 0x70) >> 4;
    switch (storage) {
    case 1:
        bytes = (bytes + default_segment_alignment - 1) & ~(default_segment_alignment - 1U);
        header->field04 = reserve_local_storage(bytes);
        break;
    case 2: case 4:
        allocation = (symbol->attributes & 0x780) >> 7;
        if (allocation == 2) break;
        if (allocation == 3) {
            if (!(flags & 0x200)) {
                symbol->attributes = (symbol->attributes & ~0x50U) | 0x20;
                break;
            }
            symbol->attributes = (symbol->attributes & 0xfffff8ffU) | 0x80;
        } else if (allocation != 1 && allocation != 4) {
            compiler_internal_error("@(#)MDmisc.c:1.87", 0x336);
            break;
        }
        reserve_segment_storage(header);
        break;
    case 3:
        offset = parameter_stack_offset;
        if (offset == 8 && ((current_symbol->symbol.field0e & 2) ||
            (!(current_symbol->symbol.field0e & 1) && code_pointer_bytes == 6))) offset += 4;
        remainder = (int)bytes % default_integer_bytes;
        header->field04 = offset;
        bytes = header->value + (remainder ? default_integer_bytes - remainder : 0);
        parameter_stack_offset = offset + bytes;
        break;
    case 5:
        if (flags & 0x20) reserve_segment_storage(header);
        break;
    case 6:
        allocation = (symbol->attributes & 0x780) >> 7;
        if (allocation == 1 || allocation == 4) reserve_segment_storage(header);
        else compiler_internal_error("@(#)MDmisc.c:1.87", 0x319);
        break;
    default:
        compiler_internal_error("@(#)MDmisc.c:1.87", 0x34e);
        break;
    }
    header->field0e |= 0x800;
    return bytes;
}
/* Original [00026e90,00026eb0): the variable storage field marks constants. */
int __fastcall is_constant_symbol(SymbolHeader *symbol)
{
    return (((TemporarySymbol *)symbol)->attributes & 0x70) == 0x40;
}
unsigned next_symbol_identifier(void);
typedef struct ComdatGroup { unsigned field00, count, field08; } ComdatGroup;
/* Original [00026eb0,00027080). */
void __fastcall update_symbol_storage(SymbolHeader *header)
{
    FunctionSymbol *symbol = (FunctionSymbol *)header;
    unsigned storage, type, identifier;
    Segment *segment;
    ComdatGroup *group;
    if (header->kind == 1 && !header->field18.identifier) {
        storage = (symbol->field24 & 0x70) >> 4;
        switch (storage) {
        case 1: case 3:
            header->field18.identifier = 6;
            break;
        case 2: case 4:
            type = (symbol->field24 & 0x780) >> 7;
            if (type == 1 || type == 4) {
                if (is_constant_symbol(header)) header->field18.identifier = 3;
                else if (header->field0e & 0x480) {
                    symbol->field24 = (symbol->field24 & ~0x50U) | 0x20;
                    header->field18.identifier = 2;
                } else {
                    symbol->field24 = (symbol->field24 & ~0x10U) | 0x60;
                    header->field18.identifier = 4;
                }
            } else if (type == 3) {
                header->field18.identifier = 2;
                header->field0e &= ~0x20;
            }
            break;
        default:
            compiler_internal_error("@(#)MDmisc.c:1.87", 0x2cd);
            break;
        }
    }
    if (!(header->field0e & 0x20)) return;
    identifier = header->field18.identifier;
    segment = (Segment *)lookup_global_symbol(identifier);
    if (segment->combine == 7) return;
    if (header->kind == 4) {
        segment = create_segment(header->name, "CODE", next_symbol_identifier());
        segment->section_number = 0;
        segment->object_attributes.coff.selection = symbol->flags52 & 0x80 ? 1 : 2;
        segment->alignment = stack_alignment;
    } else {
        if (identifier == 3) {
            segment = create_segment(header->name, "CONST", next_symbol_identifier());
            segment->section_number = 2;
        } else if (identifier == 9) {
            segment = create_segment(header->name, "TLS", next_symbol_identifier());
            segment->section_number = 11;
        } else {
            segment = create_segment(header->name, "DATA", next_symbol_identifier());
            segment->section_number = 1;
        }
        segment->object_attributes.coff.selection = 1;
        segment->alignment = default_segment_alignment;
    }
    segment->combine = 7;
    segment->format_flags = 1;
    segment->identity.owner = header;
    segment->object_attributes.coff.field45 = 0;
    group = arena_alloc(0, sizeof(ComdatGroup));
    segment->symbol.field18.pointer = group;
    group->count = 0;
    segment->parent = (Segment *)lookup_global_symbol(header->field18.identifier);
    header->field18.identifier = segment->symbol.identifier;
}

extern int cpu_model, byte_align;
/* Original [00027200,0002728d). Allocate static storage within its selected
   segment, promoting eligible wide values to eight-byte segment alignment.
   byte_align is the existing -bzalign destination at original7caac. */
void __fastcall reserve_segment_storage(SymbolHeader *symbol)
{
    Segment *segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
    unsigned bytes = symbol->value, offset;
    int alignment;
    if (!segment) compiler_internal_error("@(#)MDmisc.c:1.87", 0x35e);
    if (cpu_model > 3 && bytes >= 8 && segment->alignment == 4 &&
        !(symbol->field0e & 0x400) && !byte_align) segment->alignment = 8;
    alignment = (int)segment->alignment;
    if (bytes > 4 && !(symbol->field0e & 0x400)) {
        if (alignment >= 8) alignment = 8;
    } else if (alignment >= 4) alignment = 4;
    offset = (segment->symbol.field04 + alignment - 1) & ~(alignment - 1U);
    symbol->field04 = offset;
    segment->symbol.field04 = offset + bytes;
}
