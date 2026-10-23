#ifndef MACRO_EXPANSION_H
#define MACRO_EXPANSION_H
#include "lexer.h"
#include <recon.h>
extern char *macro_argument_start;
extern int macro_argument_state, macro_intertoken_space;
extern unsigned macro_expansion_line;
extern char macro_file_timestamp[26];
void __stdcall reference_browser_macro(MacroDefinition *definition);
void __stdcall push_macro_expansion(MacroDefinition *definition);
void rescan_macro_replacement(void);
int finish_macro_expansion(void);
void __stdcall save_macro_argument_text(char *start, char *end);
void advance_macro_argument(void);
void browser_end_macro_expansion(void);
int skip_macro_space(void);
void __stdcall expand_macro_arguments(MacroDefinition *definition, int count);
void __stdcall emit_macro_string(unsigned char delimiter, char *text);
void __stdcall emit_macro_text(char *text);
int __stdcall scan_macro_name(unsigned char first);
char *read_macro_parameter(void);
char *__stdcall copy_macro_literal(char *cursor, char *limit, unsigned char delimiter);
int __stdcall check_defined_macro(char *name);
void __stdcall macro_expansion_error(unsigned code);
long current_source_timestamp(void);
#endif
