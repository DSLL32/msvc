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
typedef struct CoffDataBlock {
    struct CoffDataBlock *next;
    unsigned char bytes[500];
    unsigned reserved;
} CoffDataBlock;
typedef struct CoffScope {
    unsigned kind, value, completed_bytes, buffered_bytes, write_count;
    CoffDataBlock *block;
} CoffScope;
typedef struct CoffContribution CoffContribution;
typedef struct CoffSection {
    CoffContribution *last;
    char *name;
    unsigned characteristics, base_offset;
    unsigned short number, state;
    unsigned char selection, reserved;
    unsigned short associated;
} CoffSection;
#pragma pack(2)
typedef struct CoffRelocation {
    unsigned address, symbol;
    unsigned short type;
} CoffRelocation;
typedef struct CoffLine {
    unsigned location;
    unsigned short line;
} CoffLine;
#pragma pack()
typedef struct CoffRelocationBlock {
    unsigned count;
    struct CoffRelocationBlock *next;
    CoffRelocation entries[50];
} CoffRelocationBlock;
typedef struct CoffLineBlock {
    unsigned count;
    struct CoffLineBlock *next;
    CoffLine entries[50];
} CoffLineBlock;
struct CoffContribution {
    CoffContribution *next;
    CoffSection *section;
    unsigned symbol_index;
    CoffScope *scope;
    CoffRelocationBlock *relocations;
    CoffLineBlock *lines;
    unsigned size, field1c, field20, field24, field28, field2c;
    unsigned short number, flags;
    unsigned field34;
};
extern CoffScope *coff_scopes, *coff_active_scope;
extern CoffDataBlock *coff_free_blocks;
extern CoffRelocationBlock *coff_free_relocations;
extern CoffLineBlock *coff_free_lines;
typedef struct CoffFilename {
    char *name;
    unsigned next_index;
} CoffFilename;
typedef struct CoffSymbolEntry {
    unsigned kind;
    union {
        CoffFilename *file;
        CoffContribution *contribution;
    } data;
} CoffSymbolEntry;
extern CoffFilename *coff_last_filename;
unsigned __fastcall coff_vector_append(CoffVector *vector, void *element);
extern unsigned short coff_machine;
extern unsigned coff_flags;
extern char *coff_source_filename;
extern FILE *coff_file;
extern void *(__fastcall *coff_allocator)(unsigned);
extern void (__fastcall *coff_error_handler)(unsigned, char *, unsigned);
extern unsigned (__cdecl *coff_clock)(void);
extern unsigned char coff_debug;
extern CoffVector *coff_sections, *coff_contributions, *coff_symbol_entries;
extern unsigned short coff_sections_locked, coff_open_sections;
extern unsigned short coff_no_section, coff_selected_section;
extern unsigned coff_symbol_count, coff_writer_phase;
extern CoffSection *coff_current_section;
extern CoffScope *coff_current_scope;
CoffVector *__fastcall coff_new_vector(unsigned short capacity);
void __fastcall coff_record_filename(char *filename);
void coff_reset_scopes(void);
CoffDataBlock *coff_new_data_block(void);
CoffScope *__fastcall coff_new_scope(unsigned kind, unsigned value);
unsigned short __fastcall coff_select_section(unsigned short identifier);
unsigned __fastcall coff_section_position(unsigned short identifier);
void __fastcall coff_fill_bytes(unsigned char value, unsigned bytes);
unsigned short __fastcall coff_create_section(char *name, unsigned flags, int definition);
unsigned short __fastcall coff_create_comdat_section(char *name, unsigned flags,
    unsigned char selection, unsigned short associated, int definition);
int __fastcall coff_open_contribution(unsigned short identifier, unsigned flags);
void *__fastcall coff_vector_get(CoffVector *vector, unsigned index);
void __fastcall coff_record_contribution(CoffContribution *contribution);
unsigned __fastcall coff_scope_position(CoffScope *scope);
void __fastcall coff_write_scope_data(const void *data, unsigned bytes);
#endif
