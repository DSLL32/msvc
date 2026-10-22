/* Frontend symbol scopes. */
#include "scopes.h"
#include "declarations.h"

SymbolScope *global_scope, *current_scope, *lookup_scope;
SymbolScope **scope_directory;

/* Original 0x50e10..0x50e66. */
void initialize_global_scope(void)
{
    global_scope = allocate_permanent(sizeof(SymbolScope));
    global_scope->buckets = allocate_permanent(4096);
    global_scope->bucket_mask = 1023;
    current_scope = global_scope;
    lookup_scope = global_scope;
    scope_directory = allocate_permanent(512);
    scope_directory[0] = global_scope;
}

CppSymbol *translation_sentinel, *translation_placeholder; /* original 0x9f1c4/c8 */
char *anonymous_symbol_name; /* original 0x9f22c */
Vector *translation_symbols; /* original 0x9f1f0 */

/* Original 0x54fd0..0x5506f. */
void initialize_translation_state(void)
{
    translation_sentinel = allocate_permanent(sizeof(CppSymbol));
    translation_sentinel->kind = 8;
    translation_sentinel->detail.value = 0;
    translation_sentinel->type = type_0000;
    translation_sentinel->attributes |= 0x20;
    translation_sentinel->flags |= 4;
    translation_sentinel->index = 1;
    translation_placeholder = allocate_permanent(sizeof(CppSymbol));
    translation_placeholder->kind = 8;
    translation_placeholder->name = anonymous_symbol_name;
    translation_placeholder->type = type_0000;
    translation_placeholder->index = 10;
    translation_placeholder->attributes |= 0x20;
    translation_placeholder->flags |= 4;
    translation_symbols = new_vector(1, 0);
}
