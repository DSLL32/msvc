#include "c2.h"
#include <recon.h>

C2ScopeHash global_scope;

void c2_reset_symbol_table(void)
{
    global_scope.previous = NULL;
    memset(global_scope.buckets, 0, sizeof(global_scope.buckets));
}

void *c2_create_type_table(void)
{
    return c2_allocate_node(1, 0);
}

C2ScopeHash *local_symbol_table;

void __fastcall insert_symbol(C2Symbol *symbol, int local)
{
    C2Symbol **bucket, *previous;
    unsigned index = symbol->serial & 1023;

    if (local)
        bucket = &local_symbol_table->buckets[index];
    else
        bucket = &global_scope.buckets[index];
    previous = *bucket;
    *bucket = symbol;
    symbol->hash_next = previous;
}
