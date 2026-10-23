#ifndef BROWSER_OUTPUT_H
#define BROWSER_OUTPUT_H
#include "parser.h"
#include <recon.h>
extern FILE *browser_output;
extern unsigned char browser_symbol_bits[8192];
char *browser_current_directory(void);
char *copy_browser_name(char *name);
typedef struct BrowserRecord {
    unsigned char kind, reserved[3];
    unsigned line, symbol_index;
    char *name;
    char *source_name;
    unsigned short attributes;
    unsigned char symbol_kind;
} BrowserRecord;
typedef char BrowserRecord_size[(sizeof(BrowserRecord) == 24) ? 1 : -1];
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
void __stdcall write_browser_record(unsigned char kind, void *value);
#endif
