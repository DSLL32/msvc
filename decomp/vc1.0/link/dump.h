#ifndef LINK_DUMP_H
#define LINK_DUMP_H
#include "library.h"
extern Image *dump_image;
extern unsigned char *dump_buffer;
extern unsigned short dump_buffer_size;
extern unsigned char dump_invalid_columns;
void process_dump_options(void);
void dump_file(const char *,const char *,unsigned char);
void initialize_disassembler(FatalRoutine,WarningRoutine);
extern unsigned long dump_file_size;
extern unsigned short dump_file_count;
extern PIMAGE_SYMBOL dump_symbol_table;
extern PIMAGE_SECTION_HEADER dump_section_headers;
void dump_headers(const char *,unsigned char);
void dump_string_table(const char *);
void dump_sections(void);
void dump_symbol_map(void);
void dump_coff_symbols(void);
#endif
