/* COFF static and inline initializer records. */
#include "output.h"
#include "coff-output.h"
#include "coff-writer.h"
#include "records.h"
#include "machine.h"
#include <recon.h>
extern int byte_block_first;
unsigned next_symbol_identifier(void);
SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol);
/* Original850c4 starts zero; retain the switch and its complete associated
   reference-segment path rather than pruning it for the present invocation. */
int coff_alias_reference_tables;
/* Original [0006d6b0,0006d9a9). Write one initializer item and return its
   contribution to the owning segment's logical position. */
unsigned __fastcall coff_output_relocation(Initializer *initializer,
    SymbolHeader *symbol, Segment *segment)
{
    unsigned bytes = 0, value, special, divisor;
    int count, width;
    SymbolHeader *referenced;
    Segment *references;
    coff_select_segment(segment);
    switch (initializer->kind) {
    case 1:
        count = (int)initializer->repeat;
        while (count > 0) {
            width = initializer->content.datum.bytes;
            if (initializer->content.datum.type == 6)
                coff_write_data(initializer->content.datum.value.bytes, width);
            else if (width == 1) {
                coff_fill_bytes(initializer->content.datum.value.byte, count);
                bytes += count;
                break;
            } else if (width == 2) coff_write_word(initializer->content.datum.value.word);
            else if (width == 4) {
                if (initializer->content.datum.type == 5)
                    coff_write_data(&initializer->content.datum.value, 4);
                else coff_write_dword(initializer->content.datum.value.integer);
            } else if (width == 8) coff_write_data(&initializer->content.datum.value, 8);
            else if (width == 10) coff_write_data(&initializer->content.datum.value, 10);
            else compiler_internal_error("@(#)coff.c:1.14", 0x314);
            bytes += width;
            --count;
        }
        return bytes;
    case 2:
        bytes = (int)(signed char)initializer->content.relocation.flags;
        referenced = lookup_global_symbol(initializer->content.relocation.symbol_id);
        if ((referenced->kind == 1 || referenced->kind == 4) &&
            (((BackendSymbol *)referenced)->attributes & 0x1000)) referenced = lookup_alias_target(referenced);
        special = 0;
        switch (referenced->kind) {
        case 1:
            value = initializer->content.relocation.offset;
            if (!byte_block_first) value += referenced->field04;
            special = (((BackendSymbol *)referenced)->attributes & 0x1000) >> 12;
            break;
        case 3:
            if (((LabelSymbol *)referenced)->label_kind != 4)
                compiler_internal_error("@(#)coff.c:1.14", 0x326);
            value = byte_block_first ? 0 : referenced->field04;
            break;
        case 4:
            value = 0;
            special = (((BackendSymbol *)referenced)->attributes & 0x1000) >> 12;
            break;
        default:
            compiler_internal_error("@(#)coff.c:1.14", 0x340);
            value = 0;
            break;
        }
        if (special && coff_alias_reference_tables) {
            coff_write_dword(value);
            if (segment->combine != 7) compiler_internal_error("@(#)coff.c:1.14", 0x345);
            if (!initializer_reference_segment) {
                references = create_segment("__CRT_F", "DEBSYM", next_symbol_identifier());
                initializer_reference_segment = references;
                references->alignment = 1;
                references->combine = 7;
                references->format_flags = 1;
                references->section_number = 1;
                references->object_attributes.coff.selection = 5;
                references->identity.owner = symbol;
                references->object_attributes.coff.field45 = 0;
                references->parent = (Segment *)lookup_global_symbol(symbol->field18.identifier);
            }
            coff_select_segment(initializer_reference_segment);
            coff_write_byte(1);
            coff_fill_bytes(0, 3);
            coff_write_dword(value);
            coff_write_dword(segment->symbol.field04 - symbol->field04);
            coff_output_bytes(0, symbol);
            coff_write_dword(value);
            coff_output_bytes(0, referenced);
            initializer_reference_segment->symbol.field04 += 0x10;
        } else {
            coff_output_bytes(0, referenced);
            coff_write_dword(value);
        }
        return bytes;
    case 4:
        divisor = (unsigned)(int)(signed char)initializer->content.byte_argument;
        bytes = coff_section_position(coff_selected_section) % divisor;
        /* The native record handler writes one zero when the remainder is
           nonzero, and returns the remainder itself. Preserve that behavior. */
        if (bytes) coff_fill_bytes(0, 1);
        return bytes;
    case 8:
        bytes = initializer->content.argument;
        coff_fill_bytes(0, bytes);
        return bytes;
    default:
        compiler_internal_error("@(#)coff.c:1.14", 0x36d);
        return 0;
    }
}
