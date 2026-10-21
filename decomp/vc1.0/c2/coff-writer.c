/* COFF writer state is introduced by its initializer, not copied from an image. */
#include "coff-writer.h"
unsigned short coff_machine;
unsigned coff_flags;
char *coff_source_filename;
FILE *coff_file;
void *(__fastcall *coff_allocator)(unsigned);
void (__fastcall *coff_error_handler)(unsigned, unsigned, void *);
unsigned (__cdecl *coff_clock)(void);
unsigned char coff_debug;
CoffVector *coff_sections, *coff_symbols, *coff_files;
unsigned short coff_scope_number, coff_section_number;
unsigned short coff_next_symbol_type, coff_symbol_type;
unsigned coff_symbol_count, coff_next_file, coff_next_section;
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
    coff_symbols = coff_new_vector(30);
    coff_files = coff_new_vector(30);
    coff_record_filename(coff_source_filename);
    coff_scope_number = 0;
    coff_section_number = 0;
    coff_symbol_count = 0;
    coff_symbol_type = coff_next_symbol_type;
    coff_next_file = 0;
    coff_next_section = 1;
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
    CoffFileEntry *entry = coff_allocator(sizeof(CoffFileEntry));
    CoffFilename *file;
    unsigned index, length, count;
    entry->kind = 1;
    file = coff_allocator(sizeof(CoffFilename));
    entry->file = file;
    file->name = filename;
    file->next_index = 0;
    index = coff_vector_append(coff_files, entry) - 1;
    if (coff_last_filename) coff_last_filename->next_index = index;
    length = 0;
    while (filename[length]) ++length;
    coff_last_filename = file;
    count = length / 18 + 1;
    entry = coff_allocator(sizeof(CoffFileEntry));
    entry->kind = 5;
    while (count--) coff_vector_append(coff_files, entry);
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
