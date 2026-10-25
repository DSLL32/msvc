#ifndef BROWSER_H
#define BROWSER_H
#include "source-input.h"
#include "macros.h"
#include <recon.h>
/* Original ring entries are 24 bytes. The common header is also used by
   queue coalescing; bytes 1 and 23 are ordinary structure padding. */
typedef struct BrowserRecordHeader {
    unsigned char kind;
    unsigned short flags;
} BrowserRecordHeader;
typedef struct BrowserRecord {
    BrowserRecordHeader header;
    unsigned line;
    unsigned symbol_index;
    char *name;
    char *name_identity;
    unsigned short attributes;
    unsigned char category;
} BrowserRecord;
typedef char BrowserRecordLayoutMustMatch[
    (sizeof(BrowserRecord) == 24 &&
     offsetof(BrowserRecordHeader, flags) == 2 &&
     offsetof(BrowserRecord, line) == 4 &&
     offsetof(BrowserRecord, symbol_index) == 8 &&
     offsetof(BrowserRecord, name) == 12 &&
     offsetof(BrowserRecord, name_identity) == 16 &&
     offsetof(BrowserRecord, attributes) == 20 &&
     offsetof(BrowserRecord, category) == 22) ? 1 : -1];
extern BrowserRecord browser_records[32];
extern int browser_record_head;
extern int browser_records_suppressed;
extern unsigned browser_last_line;
extern CppSymbol *browser_last_reference;
extern BrowserRecord browser_record_template;
extern char *browser_read_buffer;
extern FILE *browser_filtered_file;
extern int browser_symbols_overflowed;
unsigned char __stdcall deserialize_browser_record(BrowserRecord *record);
void __stdcall serialize_browser_record(BrowserRecord *record, int track_symbols);
char *browser_working_directory(void);
void __stdcall fill_browser_record(BrowserRecord *record, void *subject);
void fill_browser_filename(BrowserRecord *record);
char *duplicate_browser_text(char *text);
void suppress_browser_records(void);
void browser_compilation_error(void);
CppSymbol *__stdcall static_member_from_alias(CppSymbol *symbol);
void __stdcall set_browser_access(unsigned short *attributes, unsigned char access);
void __stdcall fill_browser_declaration(BrowserRecord *record, CppSymbol *symbol);
void __stdcall fill_browser_reference(BrowserRecord *record, CppSymbol *symbol);
void __stdcall fill_browser_symbol(BrowserRecord *record, CppSymbol *symbol);
void __stdcall fill_browser_base(BrowserRecord *record, BaseClassLink *base);
void __stdcall fill_browser_macro(BrowserRecord *record, MacroDefinition *macro);
void __stdcall fill_browser_macro_reference(BrowserRecord *record, MacroDefinition *definition);
void __stdcall fill_browser_generated_symbol(BrowserRecord *record);
BrowserRecord *__stdcall find_browser_declaration(char *name);
extern int browser_record_count;
void __stdcall append_browser_record(unsigned char kind, void *subject, unsigned line);
BrowserRecordHeader *first_browser_record(void);
BrowserRecordHeader *second_browser_record(void);
BrowserRecordHeader *__stdcall browser_record_from_end(int index);
BrowserRecordHeader *pop_last_browser_record(void);
BrowserRecordHeader *pop_first_browser_record(void);
void __stdcall emit_browser_record(BrowserRecordHeader *record);
void flush_browser_records(void);
void begin_browser_output(void);
void __stdcall format_browser_temporary_name(char *filename);
void begin_browser_records(void);
void __stdcall queue_browser_record(unsigned char kind, void *subject);
#endif
