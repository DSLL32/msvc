/* Frontend symbol scopes. */
#include "scopes.h"

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
