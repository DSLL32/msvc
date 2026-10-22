#ifndef MANGLING_H
#define MANGLING_H
#include "global-ir.h"
#define DECORATED_NAME_LIMIT 247
/* Original 0x97c38..0x98018. The following ten name-reference pointers
   start at 0x98018; they are separate from this four-name workspace. */
#define DECORATED_NAME_WORKSPACE_BYTES (4 * (DECORATED_NAME_LIMIT + 1))
extern char decorated_name_workspace[DECORATED_NAME_WORKSPACE_BYTES];
extern int decorated_type_count, decorated_name_count, mangle_name_only;
char *__stdcall mangle_symbol_name(char *cursor, CppSymbol *symbol, CppSymbol *context);
char *__stdcall format_qualified_tag_name(char *cursor, CppSymbol *tag);
#endif
