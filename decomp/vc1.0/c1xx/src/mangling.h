#ifndef MANGLING_H
#define MANGLING_H
#include "global-ir.h"
#include <recon.h>
#define DECORATED_NAME_LIMIT 247
/* Original 0x97c38..0x98018. The following ten name-reference pointers
   start at 0x98018; they are separate from this four-name workspace. */
#define DECORATED_NAME_WORKSPACE_BYTES (4 * (DECORATED_NAME_LIMIT + 1))
extern char decorated_name_workspace[DECORATED_NAME_WORKSPACE_BYTES];
extern int decorated_type_count, decorated_name_count, mangle_name_only;
extern int allow_plain_symbol_names;
extern unsigned mangle_error_sequence;
char *__stdcall mangle_symbol_name(char *cursor, CppSymbol *symbol, CppSymbol *context);
char *__stdcall format_qualified_tag_name(char *cursor, CppSymbol *tag);
unsigned __stdcall classify_symbol_decoration(CppSymbol *symbol);
char *__stdcall mangle_symbol_identifier(char *cursor, CppSymbol *symbol, unsigned encoding);
char *__stdcall mangle_symbol_scope(char *cursor, CppSymbol *symbol);
char *__stdcall mangle_symbol_encoding(char *cursor, CppSymbol *symbol,
    CppSymbol *context, unsigned encoding);
char *__stdcall mangle_identifier(char *cursor, char *name, int convert_leading_dot);
void __stdcall cache_function_parameter_bytes(CppSymbol *function);
int format_integer(unsigned value, char *cursor, unsigned radix);
#endif
