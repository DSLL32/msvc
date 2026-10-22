#ifndef MACROS_H
#define MACROS_H
#include <time.h>
#include "builtins.h"
extern char *anonymous_symbol_name;
extern char *name_defined, *name_formal, *name_ellipsis, *name_cast;
extern char *name_main, *name_winmain, *name_dllmain, *name_alignment;
extern char *name_unnamed, *name_dollar, *name_static_guard, *name_setjmp, *name_catch;
typedef struct MacroDefinition {
    struct MacroDefinition *next;
    char *name, *replacement;
    signed char parameter_count;
    unsigned char disabled;
    unsigned short field0e;
    unsigned browser_index;
} MacroDefinition;
extern MacroDefinition **macro_buckets;
extern unsigned macro_work_bytes;
extern char *macro_work_buffer, *macro_expansion_next;
extern char *macro_replacement_start, *macro_expansion_start, *macro_replacement_text;
extern short include_depth;
extern char macro_timestamp[26];
extern char *macro_date, *macro_time;
void __tzset(void);
int _isindst(struct tm *time);
long __stdcall frontend_time(long *result);
char *__stdcall format_timestamp(long timestamp);
void initialize_macro_storage(void);
void *__stdcall retry_malloc(unsigned bytes);
MacroDefinition *lookup_macro(void);
void __stdcall report_macro_error(char *directive);
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count);
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes);
void *__stdcall copy_arena_bytes(void *source, int arena, unsigned bytes);
void __stdcall browser_macro(MacroDefinition *definition);
void __stdcall define_macro(char *replacement, unsigned bytes, int parameter_count);
void __stdcall define_command_line_macro(char *definition);
int __stdcall open_source_file(char *filename, int mode);
#endif
