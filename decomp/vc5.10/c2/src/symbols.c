#include "c2.h"
#include <recon.h>

unsigned c2_symbol_hash_count;
C2Symbol *c2_symbol_hash[1024];

void c2_reset_symbol_table(void)
{
    c2_symbol_hash_count = 0;
    memset(c2_symbol_hash, 0, sizeof(c2_symbol_hash));
}

void *c2_create_type_table(void)
{
    return c2_allocate_node(1, 0);
}

/* Local symbol scopes reserve their first DWORD for separate scope state. */
typedef struct C2LocalSymbolTable {
    unsigned pending00;
    C2Symbol *buckets[1024];
} C2LocalSymbolTable;

C2LocalSymbolTable *local_symbol_table;

void __fastcall insert_symbol(C2Symbol *symbol, int local)
{
    C2Symbol **bucket, *previous;
    unsigned index = symbol->serial & 1023;

    if (local)
        bucket = &local_symbol_table->buckets[index];
    else
        bucket = &c2_symbol_hash[index];
    previous = *bucket;
    *bucket = symbol;
    symbol->hash_next = previous;
}
