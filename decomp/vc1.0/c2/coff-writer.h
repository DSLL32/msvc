#ifndef COFF_WRITER_H
#define COFF_WRITER_H
#include "coff-output.h"
typedef struct CoffVectorBlock {
    struct CoffVectorBlock *next;
    void *items[1];
} CoffVectorBlock;
typedef struct CoffVector {
    unsigned count;
    unsigned short capacity, reserved;
    unsigned used, field0c, field10, field14;
    CoffVectorBlock *first, *last;
} CoffVector;
typedef struct CoffScope CoffScope;
typedef struct CoffFilename {
    char *name;
    unsigned next_index;
} CoffFilename;
typedef struct CoffFileEntry {
    unsigned kind;
    CoffFilename *file;
} CoffFileEntry;
extern CoffFilename *coff_last_filename;
unsigned __fastcall coff_vector_append(CoffVector *vector, void *element);
extern unsigned short coff_machine;
extern unsigned coff_flags;
extern char *coff_source_filename;
extern FILE *coff_file;
extern void *(__fastcall *coff_allocator)(unsigned);
extern void (__fastcall *coff_error_handler)(unsigned, unsigned, void *);
extern unsigned (__cdecl *coff_clock)(void);
extern unsigned char coff_debug;
extern CoffVector *coff_sections, *coff_symbols, *coff_files;
extern unsigned short coff_scope_number, coff_section_number;
extern unsigned short coff_next_symbol_type, coff_symbol_type;
extern unsigned coff_symbol_count, coff_next_file, coff_next_section;
extern CoffScope *coff_current_scope;
CoffVector *__fastcall coff_new_vector(unsigned short capacity);
void __fastcall coff_record_filename(char *filename);
void coff_reset_scopes(void);
CoffScope *__fastcall coff_new_scope(unsigned kind, unsigned value);
#endif
