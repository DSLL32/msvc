/* Default storage segments and per-symbol COMDAT segments. */
#include "records.h"
#include "segments.h"
extern int default_segment_alignment, stack_alignment;
int __fastcall is_constant_symbol(SymbolHeader *symbol);
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
    if (segment->word_size == 7) return;
    if (header->kind == 4) {
        segment = create_segment(header->name, "CODE", next_symbol_identifier());
        segment->section_number = 0;
        segment->selection = symbol->flags52 & 0x80 ? 1 : 2;
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
        segment->selection = 1;
        segment->alignment = default_segment_alignment;
    }
    segment->word_size = 7;
    segment->format_flags = 1;
    segment->identity.owner = header;
    segment->field45 = 0;
    group = arena_alloc(0, sizeof(ComdatGroup));
    segment->symbol.field18.pointer = group;
    group->count = 0;
    segment->parent = (Segment *)lookup_global_symbol(header->field18.identifier);
    header->field18.identifier = segment->symbol.identifier;
}
