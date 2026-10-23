/* COFF directory, section data and symbol-table serialization. */
#include "coff-writer.h"
#include <string.h>
#include <recon.h>
CoffScope *coff_trailer_scope; /* 000851c0: optional bytes following the string table */
CoffObjectSymbol *coff_last_written_symbol; /* 000851dc */
/* Original [0006f040,0006f108); the following eight bytes are alignment.
   Line-zero entries name functions; their auxiliary record receives the
   current line-table file offset instead of rebasing the entry's symbol id. */
void __fastcall coff_write_lines(CoffContribution *contribution, unsigned base)
{
    CoffLineBlock *last = contribution->lines, *block = last;
    CoffLine *line;
    CoffSymbolEntry *symbol;
    unsigned position = contribution->lines_offset;
    unsigned count;
    unsigned char debug = (unsigned char)coff_debug;
    do {
        block = block->next;
        line = block->entries;
        for (count = 0; (int)count < (int)block->count; ++count, ++line) {
            if (!line->line && (debug & 1)) {
                symbol = coff_vector_get(coff_symbol_entries, line->location + 2);
                ((CoffFunctionAuxiliary *)symbol->data.auxiliary)->line_offset = position;
                debug = (unsigned char)coff_debug;
            } else line->location += base;
            position += sizeof(CoffLine);
        }
        count = block->count;
        if (fwrite(block->entries, sizeof(CoffLine), count, coff_file) != count)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x9d3);
        debug = (unsigned char)coff_debug;
    } while (block != last);
    block = last->next;
    last->next = coff_free_lines;
    coff_free_lines = block;
}
/* Original [0006e190,0006e220). A directory entry describes one physical
   contribution, retaining the writer's cumulative physical-address field. */
void __fastcall coff_write_section_header(CoffContribution *contribution)
{
    CoffSectionHeader header;
    coff_set_symbol_name(&header.name, contribution->section->name);
    header.virtual_address = contribution->virtual_address;
    header.physical_address = contribution->physical_address;
    header.bytes = contribution->size;
    header.data_offset = contribution->data_offset;
    header.relocations_offset = contribution->relocations_offset;
    header.lines_offset = contribution->lines_offset;
    header.relocation_count = contribution->relocation_count;
    header.line_count = contribution->line_count;
    header.characteristics = contribution->section->characteristics;
    if (fwrite(&header, sizeof(header), 1, coff_file) != 1)
        coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x829);
}
/* Original [0006e220,0006e4f0). The vector includes placeholder entries
   for file-name and section auxiliary records written by their owner entry. */
void __fastcall coff_write_symbol(CoffSymbolEntry *entry)
{
    CoffObjectSymbol symbol;
    CoffSectionAuxiliary auxiliary;
    CoffFilename *file;
    CoffContribution *contribution;
    CoffSection *section;
    unsigned length, padding;
    switch (entry->kind) {
    case 1:
        file = entry->data.file;
        strncpy(symbol.name.short_name, ".file", 8);
        symbol.value = file->next_index;
        symbol.section = 0xfffe;
        symbol.type = 0;
        symbol.storage = 103;
        length = 0;
        while (file->name[length]) ++length;
        symbol.auxiliary_count = (unsigned char)(length / 18 + 1);
        if (fwrite(&symbol, sizeof(symbol), 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x7ce);
        if (fwrite(file->name, length + 1, 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x7d2);
        padding = (unsigned)symbol.auxiliary_count * 18 - length - 1;
        while (padding--) fputc(0, coff_file);
        coff_last_written_symbol = NULL;
        return;
    case 2:
        contribution = entry->data.contribution;
        section = contribution->section;
        coff_set_symbol_name(&symbol.name, section->name);
        symbol.value = 0;
        symbol.section = contribution->number;
        symbol.type = 0;
        symbol.storage = 3;
        symbol.auxiliary_count = 1;
        if (fwrite(&symbol, sizeof(symbol), 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x7e3);
        auxiliary.bytes = contribution->size;
        auxiliary.relocation_count = contribution->relocation_count;
        auxiliary.line_count = contribution->line_count;
        auxiliary.checksum = 0;
        auxiliary.associated = 0;
        auxiliary.selection = auxiliary.reserved = 0;
        auxiliary.reserved2 = 0;
        if (section->characteristics & 0x1000) {
            auxiliary.associated = section->associated;
            auxiliary.selection = section->selection;
        }
        if (fwrite(&auxiliary, sizeof(auxiliary), 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x7ef);
        coff_last_written_symbol = NULL;
        return;
    case 3:
        coff_last_written_symbol = entry->data.symbol;
        if (fwrite(coff_last_written_symbol, sizeof(CoffObjectSymbol), 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x7f4);
        return;
    case 4:
        if (fwrite(entry->data.auxiliary, sizeof(CoffObjectSymbol), 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x7f8);
        coff_last_written_symbol = NULL;
        return;
    case 5:
        return;
    }
    compiler_internal_error("@(#)coffemit.c:1.10", 0x801);
}
/* Original [0006e0d0,0006e180). The current block closes a circular chain;
   preceding blocks are full, and only the current block may be partial. */
void __fastcall coff_flush_scope(CoffScope *scope)
{
    CoffDataBlock *last, *block, *first;
    if (!scope) return;
    last = scope->block;
    if (scope->value != 1) {
        for (block = last->next; block != last; block = block->next) {
            if (fwrite(block->bytes, 500, 1, coff_file) != 1)
                coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x952);
        }
        if (scope->buffered_bytes &&
            fwrite(last->bytes, scope->buffered_bytes, 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x957);
    }
    first = last->next;
    last->next = coff_free_blocks;
    coff_free_blocks = first;
}
/* Original [0006ddd0,0006de90). Flush the section's last contribution and
   record the file positions of its raw bytes, relocations and line records. */
void __fastcall coff_flush_section(CoffSection *section)
{
    CoffContribution *contribution = section->last;
    CoffScope *scope = contribution->scope;
    unsigned bytes;
    if (scope->value == 1) contribution->data_offset = 0;
    else contribution->data_offset = ftell(coff_file);
    contribution->virtual_address = 0;
    contribution->physical_address = coff_data_offset;
    bytes = coff_scope_position(scope);
    contribution->size = bytes;
    coff_flush_scope(scope);
    coff_data_offset += bytes;
    if (contribution->relocation_count) {
        contribution->relocations_offset = ftell(coff_file);
        coff_write_relocations(contribution, contribution->virtual_address);
    } else contribution->relocations_offset = 0;
    if (contribution->line_count) {
        contribution->lines_offset = ftell(coff_file);
        coff_write_lines(contribution, contribution->virtual_address);
    } else contribution->lines_offset = 0;
    contribution->flags |= 1;
}
/* Original [0006dd00,0006ddc0). Freeze the contribution directory before
   raw data are written. A nonzero count reserves additional logical sections. */
void __fastcall coff_reserve_sections(unsigned short count)
{
    unsigned total, contributions;
    if (coff_reserved_sections) return;
    if (!count) total = coff_vector_count(coff_contributions);
    else {
        if (coff_vector_count(coff_sections) > count) {
            coff_error_handler(7, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x2c3);
            goto position_data;
        }
        contributions = coff_vector_count(coff_contributions);
        total = count + contributions - coff_vector_count(coff_sections);
        if (total > 0xffff) {
            coff_error_handler(6, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x2c8);
            goto position_data;
        }
    }
    coff_reserved_sections = (unsigned short)total;
position_data:
    fseek(coff_file, (unsigned)coff_reserved_sections * sizeof(CoffSectionHeader) + sizeof(CoffFileHeader), SEEK_SET);
}
/* Original [0006def0,0006e0d0). Data precede the symbol table; the reserved
   file/section directory is filled after symbol names have been collected. */
void coff_finalize(void)
{
    CoffFileHeader header;
    CoffSection *section;
    CoffSymbolEntry *symbol;
    CoffContribution *contribution;
    unsigned string_bytes;
    if (!coff_reserved_sections) coff_reserve_sections(0);
    fseek(coff_file, (unsigned)coff_reserved_sections * sizeof(CoffSectionHeader) + sizeof(CoffFileHeader), SEEK_SET);
    while ((section = coff_vector_next(coff_sections)) != NULL) {
        if ((section->state & 1) && !(section->state & 4)) coff_flush_section(section);
    }
    header.machine = coff_machine;
    header.sections = (unsigned short)coff_vector_count(coff_contributions);
    header.timestamp = coff_clock();
    header.optional_bytes = header.characteristics = 0;
    header.symbols = coff_vector_count(coff_symbol_entries);
    header.symbols_offset = ftell(coff_file);
    while ((symbol = coff_vector_next(coff_symbol_entries)) != NULL) coff_write_symbol(symbol);
    fseek(coff_file, 0, SEEK_SET);
    if (fwrite(&header, sizeof(header), 1, coff_file) != 1)
        coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x343);
    ftell(coff_file); /* The original retains this position query. */
    while ((contribution = coff_vector_next(coff_contributions)) != NULL)
        coff_write_section_header(contribution);
    fseek(coff_file, 0, SEEK_END);
    string_bytes = coff_scope_position(coff_current_scope) + 4;
    if (fwrite(&string_bytes, sizeof(string_bytes), 1, coff_file) != 1)
        coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x353);
    coff_flush_scope(coff_current_scope);
    if (coff_trailer_scope) {
        fseek(coff_file, 0, SEEK_END);
        coff_flush_scope(coff_trailer_scope);
    }
}

/* Original [0006f110,0006f187). Relocation blocks form a tail-owned ring.
   Emit each occupied block and splice the complete ring onto the free list. */
void __fastcall coff_write_relocations(CoffContribution *contribution, unsigned base)
{
    CoffRelocationBlock *tail, *block;
    unsigned i;
    tail = contribution->relocations;
    block = tail;
    do {
        block = block->next;
        for (i = block->count; i; --i)
            block->entries[i - 1].address += base;
        if (fwrite(block->entries, sizeof(CoffRelocation), block->count, coff_file)
            != block->count)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x9a9);
    } while (block != tail);
    block = tail->next;
    tail->next = coff_free_relocations;
    coff_free_relocations = block;
}
