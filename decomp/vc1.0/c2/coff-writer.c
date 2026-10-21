/* COFF writer state is introduced by its initializer, not copied from an image. */
#include "coff-writer.h"
unsigned short coff_machine;
unsigned coff_flags;
char *coff_source_filename;
FILE *coff_file;
void *(__fastcall *coff_allocator)(unsigned);
void (__fastcall *coff_error_handler)(unsigned, char *, unsigned);
unsigned (__cdecl *coff_clock)(void);
unsigned char coff_debug;
CoffVector *coff_sections, *coff_contributions, *coff_symbol_entries;
unsigned short coff_sections_locked, coff_open_sections;
unsigned short coff_no_section, coff_selected_section;
unsigned coff_symbol_count, coff_writer_phase;
CoffSection *coff_current_section;
CoffScope *coff_current_scope;
/* Original [0006daa0,0006db80). */
int __fastcall coff_initialize(CoffConfiguration *configuration)
{
    if (configuration->version != 1) return 0;
    coff_machine = configuration->machine;
    coff_source_filename = configuration->source;
    coff_file = configuration->file;
    coff_flags = configuration->flags;
    coff_allocator = configuration->allocate;
    coff_error_handler = configuration->error;
    coff_clock = configuration->timestamp;
    coff_debug = configuration->debug;
    coff_sections = coff_new_vector(30);
    coff_contributions = coff_new_vector(30);
    coff_symbol_entries = coff_new_vector(30);
    coff_record_filename(coff_source_filename);
    coff_sections_locked = 0;
    coff_open_sections = 0;
    coff_symbol_count = 0;
    coff_selected_section = coff_no_section;
    coff_current_section = 0;
    coff_writer_phase = 1;
    coff_reset_scopes();
    coff_current_scope = coff_new_scope(1, 0);
    return 1;
}

/* Original [0006dca0,0006dd00). */
CoffVector *__fastcall coff_new_vector(unsigned short capacity)
{
    CoffVector *vector = coff_allocator(sizeof(CoffVector));
    vector->capacity = capacity;
    vector->used = 0;
    vector->count = 0;
    vector->first = coff_allocator((unsigned)capacity * sizeof(void *) + 8);
    vector->first->items[0] = 0;
    vector->last = vector->first;
    vector->field0c = 0;
    vector->field10 = 0;
    vector->field14 = 0;
    return vector;
}

CoffFilename *coff_last_filename; /* 000851d8 */
/* Original [0006dc00,0006dca0). */
void __fastcall coff_record_filename(char *filename)
{
    CoffSymbolEntry *entry = coff_allocator(sizeof(CoffSymbolEntry));
    CoffFilename *file;
    unsigned index, length, count;
    entry->kind = 1;
    file = coff_allocator(sizeof(CoffFilename));
    entry->data.file = file;
    file->name = filename;
    file->next_index = 0;
    index = coff_vector_append(coff_symbol_entries, entry) - 1;
    if (coff_last_filename) coff_last_filename->next_index = index;
    length = 0;
    while (filename[length]) ++length;
    coff_last_filename = file;
    count = length / 18 + 1;
    entry = coff_allocator(sizeof(CoffSymbolEntry));
    entry->kind = 5;
    while (count--) coff_vector_append(coff_symbol_entries, entry);
}

/* Original [0006e530,0006e580). Return the one-based appended index. */
unsigned __fastcall coff_vector_append(CoffVector *vector, void *element)
{
    CoffVectorBlock *block;
    if (vector->used >= vector->capacity) {
        block = coff_allocator((unsigned)vector->capacity * sizeof(void *) + 8);
        block->next = vector->last->next;
        vector->last->next = block;
        vector->last = block;
        vector->used = 0;
    }
    vector->last->items[vector->used] = element;
    ++vector->used;
    return ++vector->count;
}

CoffScope *coff_scopes, *coff_active_scope; /* 000851b4, 000851bc */
CoffDataBlock *coff_free_blocks; /* 000851c4 */
CoffRelocationBlock *coff_free_relocations; /* 000851c8 */
CoffLineBlock *coff_free_lines; /* 000851cc */
/* Original [0006dbc0,0006dc00). */
void coff_reset_scopes(void)
{
    coff_scopes = 0;
    coff_current_scope = 0;
    coff_active_scope = 0;
    coff_free_blocks = 0;
    coff_free_relocations = 0;
    coff_free_lines = 0;
}

/* Original [0006db80,0006dbc0). */
CoffScope *__fastcall coff_new_scope(unsigned kind, unsigned value)
{
    CoffScope *scope = coff_allocator(sizeof(CoffScope));
    CoffDataBlock *block = coff_new_data_block();
    block->next = block;
    scope->kind = kind;
    scope->value = value;
    scope->completed_bytes = 0;
    scope->buffered_bytes = 0;
    scope->write_count = 0;
    scope->block = block;
    return scope;
}

/* Original [0006f1a0,0006f1c0). */
CoffDataBlock *coff_new_data_block(void)
{
    CoffDataBlock *block = coff_free_blocks;
    if (block) {
        coff_free_blocks = block->next;
        return block;
    }
    return coff_allocator(sizeof(CoffDataBlock));
}

/* Original [0006e4f0,0006e530). */
unsigned short __fastcall coff_create_section(char *name, unsigned flags, int definition)
{
    CoffSection *section = coff_allocator(sizeof(CoffSection));
    unsigned index;
    if (definition) section->state |= 8;
    section->characteristics = flags;
    index = coff_vector_append(coff_sections, section);
    section->number = (unsigned short)index;
    section->name = name;
    return (unsigned short)index;
}

/* Original [0006e5d0,0006e730). Start a contribution to a section. */
int __fastcall coff_open_contribution(unsigned short identifier, unsigned flags)
{
    CoffSection *section = coff_vector_get(coff_sections, identifier);
    CoffContribution *previous, *contribution;
    CoffRelocationBlock *relocations;
    CoffLineBlock *lines;
    unsigned short state;
    if (!section) return 0;
    state = section->state;
    if (state & 2) return 0;
    if ((state & 1) && ((section->characteristics & 0x1000) || coff_sections_locked)) return 0;
    if (coff_writer_phase == 3 && coff_open_sections >= 1) return 0;
    previous = section->last;
    if (!previous || (previous->flags & 1)) {
        contribution = coff_allocator(sizeof(CoffContribution));
        contribution->flags = 0;
        contribution->scope = coff_new_scope((state & 4) == 0,
            flags + 0x3fffff80U < 1U);
        relocations = coff_allocator(sizeof(CoffRelocationBlock));
        relocations->count = 0;
        relocations->next = relocations;
        contribution->relocations = relocations;
        lines = coff_allocator(sizeof(CoffLineBlock));
        lines->count = 0;
        lines->next = lines;
        contribution->lines = lines;
        contribution->section = section;
        if (previous) {
            contribution->next = previous->next;
            previous->next = contribution;
            section->last = contribution;
            section->base_offset += previous->size;
        } else {
            contribution->next = contribution;
            section->last = contribution;
            section->base_offset = 0;
        }
        contribution->number = (unsigned short)coff_vector_append(coff_contributions, contribution);
        coff_record_contribution(contribution);
    }
    state |= 1;
    section->state = state;
    state |= 2;
    section->state = state;
    ++coff_open_sections;
    return 1;
}

/* Original [0006e780,0006e7d0). Entries have one-based indexes. */
void *__fastcall coff_vector_get(CoffVector *vector, unsigned index)
{
    unsigned capacity, limit;
    CoffVectorBlock *block;
    if (vector->count < index) return 0;
    capacity = vector->capacity;
    block = vector->first;
    limit = capacity;
    while (limit < index) {
        if (!block) compiler_internal_error("@(#)coffemit.c:1.10", 0xa37);
        block = block->next;
        limit += capacity;
    }
    index -= limit - capacity;
    return index ? block->items[index - 1] : block->next;
}

/* Original [0006e730,0006e780). A section symbol and its auxiliary record. */
void __fastcall coff_record_contribution(CoffContribution *contribution)
{
    CoffSymbolEntry *entry = coff_allocator(sizeof(CoffSymbolEntry));
    entry->kind = 2;
    entry->data.contribution = contribution;
    contribution->symbol_index = coff_vector_append(coff_symbol_entries, entry) - 1;
    entry = coff_allocator(sizeof(CoffSymbolEntry));
    entry->kind = 5;
    coff_vector_append(coff_symbol_entries, entry);
}

/* Original [0006e830,0006e890). Return the previously selected section. */
unsigned short __fastcall coff_select_section(unsigned short identifier)
{
    CoffSection *section = coff_vector_get(coff_sections, identifier);
    unsigned short previous;
    if (!section || !(section->state & 2)) return coff_no_section;
    previous = coff_selected_section;
    coff_selected_section = identifier;
    coff_current_section = section;
    coff_active_scope = section->last->scope;
    return previous;
}

/* Original [0006edc0,0006ee50). A zero identifier denotes the current section. */
unsigned __fastcall coff_section_position(unsigned short identifier)
{
    CoffSection *section;
    if (identifier == coff_no_section) {
        section = coff_current_section;
        if (coff_vector_get(coff_sections, coff_selected_section) != section)
            compiler_internal_error("@(#)coffemit.c:1.10", 0x67f);
    } else section = coff_vector_get(coff_sections, identifier);
    if (!section) {
        compiler_internal_error("@(#)coffemit.c:1.10", 0x688);
        return 0;
    }
    if (!(section->state & 1)) return 0;
    return coff_scope_position(section->last->scope) + section->base_offset;
}

/* Original [0006e180,0006e190). */
unsigned __fastcall coff_scope_position(CoffScope *scope)
{
    return scope->completed_bytes + scope->buffered_bytes;
}

/* Original [0006ee90,0006efd0). Write a record into the active data scope. */
void __fastcall coff_write_scope_data(const void *data, unsigned bytes)
{
    CoffScope *scope = coff_active_scope;
    const unsigned char *source = data;
    CoffDataBlock *block;
    unsigned count, i;
    if (scope->value == 1) {
        scope->buffered_bytes += bytes;
        ++scope->write_count;
        return;
    }
    if (!scope->kind) {
        if (fwrite(data, bytes, 1, coff_file) != 1)
            coff_error_handler(14, "d:\\usr\\cuda\\src\\P2\\coffemit.c", 0x91d);
        scope->buffered_bytes += bytes;
        ++scope->write_count;
        return;
    }
    while (500 - scope->buffered_bytes < bytes) {
        count = 500 - scope->buffered_bytes;
        for (i = 0; i < count; ++i)
            scope->block->bytes[scope->buffered_bytes + i] = source[i];
        bytes -= count;
        source += count;
        scope->buffered_bytes += count;
        if (scope->buffered_bytes != 500)
            compiler_internal_error("@(#)coffemit.c:1.10", 0x928);
        block = coff_new_data_block();
        block->next = scope->block->next;
        scope->block->next = block;
        scope->block = block;
        scope->completed_bytes += scope->buffered_bytes;
        scope->buffered_bytes = 0;
    }
    for (i = 0; i < bytes; ++i)
        scope->block->bytes[scope->buffered_bytes + i] = source[i];
    scope->buffered_bytes += bytes;
    ++scope->write_count;
}

/* Original [0006efd0,0006eff0). */
void __fastcall coff_write_byte(unsigned char value)
{
    coff_write_scope_data(&value, 1);
}
