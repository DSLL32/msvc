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

C2Symbol *__fastcall lookup_global_symbol(unsigned serial)
{
    C2Symbol *symbol;
    unsigned bucket;

    if (!serial)
        return NULL;
    bucket = serial & 1023;
    symbol = global_scope.buckets[bucket];
    while (symbol) {
        if (symbol->serial == serial)
            return symbol;
        symbol = symbol->hash_next;
    }
    if (local_symbol_table) {
        symbol = local_symbol_table->buckets[bucket];
        while (symbol) {
            if (symbol->serial == serial)
                return symbol;
            symbol = symbol->hash_next;
        }
    }
    return NULL;
}

unsigned next_symbol_serial(void)
{
    if (!c2_il_initial_symbol_count)
        c2_fatal(0x37);
    return c2_il_initial_symbol_count++;
}
