#ifndef COFF_WRITER_H
#define COFF_WRITER_H
#include "coff-output.h"
#include <recon.h>
typedef struct CoffVectorBlock {
    struct CoffVectorBlock *next;
    void *items[1];
} CoffVectorBlock;
typedef struct CoffVector {
    unsigned count;
    unsigned short capacity, reserved;
    unsigned used, iterator_count, iterator_slot;
    CoffVectorBlock *iterator_block;
    CoffVectorBlock *first, *last;
} CoffVector;
typedef char CoffVectorSizeCheck[sizeof(CoffVector) == 32 ? 1 : -1];
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
typedef union CoffName {
    char short_name[8];
    struct { unsigned zero, offset; } long_name;
} CoffName;
typedef struct CoffFileHeader {
    unsigned short machine, sections;
    unsigned timestamp, symbols_offset, symbols;
    unsigned short optional_bytes, characteristics;
} CoffFileHeader;
typedef struct CoffSectionHeader {
    CoffName name;
    unsigned physical_address, virtual_address, bytes;
    unsigned data_offset, relocations_offset, lines_offset;
    unsigned short relocation_count, line_count;
    unsigned characteristics;
} CoffSectionHeader;
typedef struct CoffSectionAuxiliary {
    unsigned bytes;
    unsigned short relocation_count, line_count;
    unsigned checksum;
    unsigned short associated;
    unsigned char selection, reserved;
    unsigned short reserved2;
} CoffSectionAuxiliary;
typedef struct CoffRelocation {
    unsigned address, symbol;
    unsigned short type;
} CoffRelocation;
typedef struct CoffLine {
    unsigned location;
    unsigned short line;
} CoffLine;
typedef struct CoffLineAuxiliary {
    unsigned reserved0;
    unsigned short line, reserved6;
    unsigned reserved8, next;
    unsigned short reserved16;
} CoffLineAuxiliary;
typedef struct CoffFunctionAuxiliary {
    unsigned tag, bytes, line_offset, next;
    unsigned short reserved16;
} CoffFunctionAuxiliary;
typedef struct CoffObjectSymbol {
    CoffName name;
    unsigned value;
    unsigned short section, type;
    unsigned char storage, auxiliary_count;
} CoffObjectSymbol;
#pragma pack()
typedef char CoffObjectSymbolSizeCheck[sizeof(CoffObjectSymbol) == 18 ? 1 : -1];
typedef char CoffFileHeaderSizeCheck[sizeof(CoffFileHeader) == 20 ? 1 : -1];
typedef char CoffSectionHeaderSizeCheck[sizeof(CoffSectionHeader) == 40 ? 1 : -1];
typedef char CoffSectionAuxiliarySizeCheck[sizeof(CoffSectionAuxiliary) == 18 ? 1 : -1];
typedef struct CoffRelocationBlock {
    unsigned count;
    struct CoffRelocationBlock *next;
    CoffRelocation entries[50];
} CoffRelocationBlock;
typedef struct CoffFunctionFixup {
    unsigned identifier;
    CoffRelocation *relocation;
    struct CoffFunctionFixup *next;
} CoffFunctionFixup;
typedef struct CoffLineBlock {
    unsigned count;
    struct CoffLineBlock *next;
    CoffLine entries[50];
} CoffLineBlock;
typedef char CoffLineBlockSizeCheck[sizeof(CoffLineBlock) == 308 ? 1 : -1];
typedef char CoffLineAuxiliarySizeCheck[sizeof(CoffLineAuxiliary) == 18 ? 1 : -1];
typedef char CoffFunctionAuxiliarySizeCheck[sizeof(CoffFunctionAuxiliary) == 18 ? 1 : -1];
struct CoffContribution {
    CoffContribution *next;
    CoffSection *section;
    unsigned symbol_index;
    CoffScope *scope;
    CoffRelocationBlock *relocations;
    CoffLineBlock *lines;
    unsigned size, virtual_address, physical_address, data_offset, relocations_offset, lines_offset;
    unsigned short number, flags;
    unsigned short relocation_count, line_count;
};
typedef char CoffContributionSizeCheck[sizeof(CoffContribution) == 56 ? 1 : -1];
extern CoffScope *coff_scopes, *coff_active_scope;
extern CoffDataBlock *coff_free_blocks;
extern CoffRelocationBlock *coff_free_relocations;
extern CoffLineBlock *coff_free_lines;
extern CoffFunctionFixup *coff_function_fixups[32];
typedef struct CoffFilename {
    char *name;
    unsigned next_index;
} CoffFilename;
typedef struct CoffSymbolEntry {
    unsigned kind;
    union {
        CoffFilename *file;
        CoffContribution *contribution;
        CoffObjectSymbol *symbol;
        void *auxiliary;
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
extern unsigned short coff_reserved_sections, coff_open_sections;
extern unsigned short coff_no_section, coff_selected_section;
extern unsigned coff_data_offset, coff_writer_phase;
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
unsigned __fastcall coff_define_function(unsigned identifier, char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, short has_lines);
unsigned __fastcall coff_define_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type);
unsigned __fastcall coff_define_weak_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, unsigned fallback,
    unsigned char search);
unsigned __fastcall coff_define_aux_symbol(char *name, unsigned short section,
    unsigned char storage, unsigned offset, unsigned short type, void **auxiliary);
void __fastcall coff_resolve_function(unsigned identifier, unsigned handle);
void __fastcall coff_set_symbol_name(CoffName *destination, char *name);
unsigned __fastcall coff_data_position(CoffScope *scope);
void __fastcall coff_begin_lines(unsigned handle, unsigned short line,
    unsigned bytes, unsigned offset);
void __fastcall coff_line_marker(unsigned short marker);
void __fastcall coff_record_line(unsigned short line, unsigned offset);
void __fastcall coff_end_lines(unsigned short line, unsigned offset);
void __fastcall coff_set_section_flags(unsigned short section, unsigned flags);
void coff_close_all_sections(void);
void coff_finalize(void);
void *__fastcall coff_vector_next(CoffVector *vector);
unsigned __fastcall coff_vector_count(CoffVector *vector);
void __fastcall coff_reserve_sections(unsigned short count);
void __fastcall coff_flush_section(CoffSection *section);
void __fastcall coff_flush_scope(CoffScope *scope);
void __fastcall coff_write_symbol(CoffSymbolEntry *entry);
void __fastcall coff_write_section_header(CoffContribution *contribution);
void __fastcall coff_write_relocations(CoffContribution *contribution, unsigned base);
void __fastcall coff_write_lines(CoffContribution *contribution, unsigned base);
extern CoffScope *coff_trailer_scope;
extern CoffObjectSymbol *coff_last_written_symbol;
void __fastcall coff_write_word(unsigned short value);
void __fastcall coff_write_dword(unsigned value);
void __fastcall coff_add_relocation(unsigned identifier, unsigned offset, unsigned handle, unsigned short type);
void __fastcall coff_add_function_fixup(unsigned identifier, CoffRelocation *relocation);
unsigned __fastcall coff_section_symbol(unsigned short identifier);
#endif
