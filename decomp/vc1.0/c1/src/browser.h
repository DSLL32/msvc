#ifndef BROWSER_OUTPUT_H
#define BROWSER_OUTPUT_H
#include <stddef.h>
#include "parser.h"
#include <recon.h>
extern FILE *browser_output;
extern unsigned char browser_symbol_bits[8192];
char *browser_current_directory(void);
char *copy_browser_name(char *name);
extern char *browser_read_name;
extern FILE *browser_filtered_output;
extern int browser_unfiltered;
typedef struct BrowserRecord {
    unsigned char kind, reserved[3];
    unsigned line, symbol_index;
    char *name;
    char *source_name;
    unsigned short attributes;
    unsigned char symbol_kind;
} BrowserRecord;
typedef char BrowserRecord_size[(sizeof(BrowserRecord) == 24) ? 1 : -1];
typedef char BrowserRecord_line[(offsetof(BrowserRecord,line) == 4) ? 1 : -1];
typedef char BrowserRecord_index[(offsetof(BrowserRecord,symbol_index) == 8) ? 1 : -1];
typedef char BrowserRecord_name[(offsetof(BrowserRecord,name) == 12) ? 1 : -1];
typedef char BrowserRecord_source[(offsetof(BrowserRecord,source_name) == 16) ? 1 : -1];
typedef char BrowserRecord_attrs[(offsetof(BrowserRecord,attributes) == 20) ? 1 : -1];
typedef char BrowserRecord_kind[(offsetof(BrowserRecord,symbol_kind) == 22) ? 1 : -1];
extern int browser_suppressed, browser_last_line;
extern FrontendSymbol *browser_last_reference;
extern unsigned browser_pending_flags;
extern BrowserRecord browser_line_template;
void __stdcall serialize_browser_record(BrowserRecord *record, int track_symbols);
void __stdcall prepare_browser_record(BrowserRecord *record, void *value);
void __stdcall append_browser_record(BrowserRecord *record);
void prepare_browser_file_record(BrowserRecord *record);
void __stdcall prepare_browser_definition(BrowserRecord *record, FrontendSymbol *symbol);
void __stdcall prepare_browser_reference(BrowserRecord *record, FrontendSymbol *symbol);
void __stdcall prepare_browser_symbol_record(BrowserRecord *record, FrontendSymbol *symbol);
void __stdcall prepare_browser_macro_definition(BrowserRecord *record, MacroDefinition *macro);
void __stdcall prepare_browser_macro_reference(BrowserRecord *record, MacroDefinition *macro);
void __stdcall prepare_browser_anonymous_record(BrowserRecord *record);
void __stdcall make_browser_temporary_name(char *name);
void write_browser_header(void);
void browser_error_record(void);
void note_browser_error(void);
unsigned char __stdcall read_browser_record(BrowserRecord *record);
char *read_browser_string(void);
void __stdcall write_browser_record(unsigned char kind, void *value);
#endif
