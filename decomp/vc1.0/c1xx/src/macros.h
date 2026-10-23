#ifndef MACROS_H
#define MACROS_H
#include "declarations.h"
#include "parser-state.h"
#include <recon.h>

typedef struct SpecialIdentifier {
    char **destination;
    char *spelling;
} SpecialIdentifier;
extern SpecialIdentifier special_identifiers[];
extern char *name_return_udt, *name_this, *name_that;
extern char *name_vftable, *name_vbtable, *name_vfptr, *name_vbptr, *name_vdispmap;
extern char *name_init_vbases, *name_flags, *name_dtor, *name_vbase_dtor;
extern char *name_delete_dtor, *name_vector_delete_dtor, *name_assignment;
extern char *name_most_derived, *name_new, *name_delete, *name_ctor;
extern char *name_default_ctor_closure, *name_static_destructor;
extern char *name_vector_ctor, *name_vector_ctor_vb, *name_vector_dtor;
extern char *name_new_target, *name_atexit, *name_tlregdtor;
extern char *name_defined, *name_formal, *name_ellipsis, *name_cast;
extern char *name_main, *name_winmain, *name_dllmain, *name_alignment;
extern char *name_unnamed, *name_dollar, *name_static_guard, *name_setjmp, *name_catch;
extern char macro_timestamp[26];
extern char *macro_date, *macro_time;
typedef struct MacroDefinition {
    struct MacroDefinition *next;
    char *name, *replacement;
    signed char parameter_count;
    unsigned char flags;
    unsigned short field0e;
    unsigned browser_index;
} MacroDefinition;
extern MacroDefinition **macro_buckets;
extern unsigned macro_work_bytes;
extern char *macro_work_buffer;
extern char *macro_replacement_start, *macro_expansion_start;
extern char *macro_replacement_text;
extern int macro_formal_count;
void read_macro_parameters(void);
unsigned read_macro_replacement(void);
extern short include_depth;
extern unsigned browser_symbol_limit;
MacroDefinition *lookup_macro(void);
void __stdcall report_macro_error(char *directive);
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count);
int __stdcall macro_replacement_diff(char *left, char *right, unsigned bytes);
void *__stdcall copy_arena_bytes(void *source, int arena, unsigned bytes);
void __stdcall browser_macro(MacroDefinition *definition);
void *__stdcall retry_malloc(unsigned bytes);
long __stdcall frontend_time(long *result);
char *__stdcall format_timestamp(long timestamp);
void initialize_macro_storage(void);
void __stdcall define_macro(char *replacement, unsigned bytes, int parameter_count);
void __stdcall define_command_line_macro(char *definition);
int __stdcall open_source_file(char *filename, int mode);
typedef char MacroDefinitionSizeMustBe20[(sizeof(MacroDefinition) == 20) ? 1 : -1];
#endif
