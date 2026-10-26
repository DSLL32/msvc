#include "c2.h"
#include "output-callbacks.h"
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

C2Symbol *__fastcall put_symbol(C2ScopeHash *scope, C2Symbol *symbol)
{
    unsigned bucket = symbol->serial & 1023;
    C2Symbol *previous = scope->buckets[bucket];
    scope->buckets[bucket] = symbol;
    symbol->hash_next = previous;
    return symbol;
}

C2Symbol *__fastcall lookup_symbol(C2ScopeHash *scope, unsigned serial)
{
    C2Symbol *symbol;
    for (; scope; scope = scope->previous) {
        for (symbol = scope->buckets[serial & 1023]; symbol; symbol = symbol->hash_next) {
            if (symbol->serial == serial) return symbol;
        }
    }
    return NULL;
}

void __fastcall activate_scope(C2ScopeHash *scope)
{
    if (scope->previous != &global_scope)
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 1593);
    local_symbol_table = scope;
}

/* The iterator validates continuation against its last scope and symbol.
 * A NULL previous argument starts a new walk, so no initial-state assumption
 * is made about these three cells before that explicit transition. */
static C2ScopeHash *iterated_scope; /* 491090 */
static C2Symbol *iterated_symbol; /* 491098 */
static int iterated_bucket; /* 4910a0 */

C2Symbol *__fastcall next_scope_symbol(C2ScopeHash *scope, C2Symbol *previous)
{
    if (!previous) {
        iterated_symbol = NULL;
        iterated_bucket = -1;
        iterated_scope = scope;
    }
    if (previous != iterated_symbol || scope != iterated_scope)
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 2101);
    if (previous && previous->hash_next) {
        iterated_symbol = previous->hash_next;
        return iterated_symbol;
    }
    for (++iterated_bucket; iterated_bucket < 1024; ++iterated_bucket) {
        if (scope->buckets[iterated_bucket]) {
            iterated_symbol = scope->buckets[iterated_bucket];
            return iterated_symbol;
        }
    }
    return NULL;
}

void __fastcall visit_symbols(int local, C2AssemblyVisitor visitor)
{
    C2ScopeHash *scope = local ? local_symbol_table : &global_scope;
    C2Symbol *symbol;

    if (!scope) return;
    for (symbol = next_scope_symbol(scope, NULL); symbol;
         symbol = next_scope_symbol(scope, symbol)) {
        visitor(symbol);
    }
}

int __fastcall is_known_symbol(unsigned serial)
{
    if (serial >= 0x3f0000 && serial <= 0x3fffff) return 0;
    if (serial < (unsigned)c2_il_symbol_count) return 1;
    return lookup_symbol(&global_scope, serial) != NULL;
}

/* Generated labels are ordinary local kind3 symbols in the active scope. */
C2InputSymbol *new_flow_label(void)
{
    C2InputSymbol *label = c2_allocate_node(3, 1);
    label->symbol.source_kind = 0x20;
    label->symbol.serial = next_symbol_serial();
    insert_symbol(&label->symbol, 1);
    return label;
}
