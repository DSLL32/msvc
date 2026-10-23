/* Resolve dependencies of static initializer records. */
#include "machine.h"
#include "initializers.h"
#include "records.h"
#include "symbol-output.h"
#include "functions.h"
#include <recon.h>
SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol);
int __fastcall initializer_symbols_available(SymbolHeader *symbol);
void __fastcall queue_initializer_dependencies(SymbolHeader *symbol);
BackendSymbolList *new_symbol_list_node(void);
/* Original [0004eec0,0004efc0). */
BackendSymbolList *collect_global_symbols(void)
{
    Initializer *initializer;
    SymbolHeader *symbol;
    BackendSymbol *storage;
    BackendSymbolList *symbols = 0, **tail = &symbols, *entry;
    unsigned short state;
    for (initializer = global_initializers; initializer; initializer = initializer->content.reference.link) {
        symbol = lookup_global_symbol(initializer->content.reference.symbol_id);
        if (!symbol) continue;
        storage = (BackendSymbol *)symbol;
        if ((symbol->kind == 1 || symbol->kind == 4) && (storage->attributes & 0x1000))
            symbol = lookup_alias_target(symbol);
        storage = (BackendSymbol *)symbol;
        state = storage->output_flags;
        if (state & 3) continue;
        if (!(state & 4)) {
            if (symbol->field0e & 0x20) continue;
            storage->output_flags |= 4;
            if (!initializer_symbols_available(symbol)) {
                storage->output_flags &= ~4;
                continue;
            }
        }
        queue_initializer_dependencies(symbol);
    }
    for (initializer = global_initializers; initializer; initializer = initializer->content.reference.link) {
        symbol = lookup_global_symbol(initializer->content.reference.symbol_id);
        if (!symbol) continue;
        storage = (BackendSymbol *)symbol;
        if ((symbol->kind == 1 || symbol->kind == 4) && (storage->attributes & 0x1000))
            symbol = lookup_alias_target(symbol);
        storage = (BackendSymbol *)symbol;
        state = storage->output_flags;
        if ((state & 4) && !(state & 3)) {
            entry = new_symbol_list_node();
            entry->symbol = symbol;
            entry->next = 0;
            *tail = entry;
            tail = &entry->next;
        }
    }
    return symbols;
}
/* Original [0004ee20,0004eebe). Queue a static symbol and recursively
   queue symbol-valued initializer references not already under consideration. */
void __fastcall queue_initializer_dependencies(SymbolHeader *symbol)
{
    Initializer *initializer;
    SymbolHeader *referenced;
    if (((BackendSymbol *)symbol)->output_flags & 3) return;
    queue_symbol_debug(symbol);
    if (symbol->kind == 1) {
        if (!(symbol->field0e & 0x480)) return;
        initializer = find_initializer(symbol->identifier)->next;
        for (; initializer; initializer = initializer->next) {
            if (initializer->kind != 2) continue;
            referenced = lookup_global_symbol(initializer->content.relocation.symbol_id);
            if ((referenced->kind == 1 || referenced->kind == 4) &&
                (((BackendSymbol *)referenced)->attributes & 0x1000)) referenced = lookup_alias_target(referenced);
            if (!(((BackendSymbol *)referenced)->output_flags & 4)) queue_initializer_dependencies(referenced);
        }
    } else if (symbol->kind == 4) {
        if (((FunctionSymbol *)symbol)->field24 & 0x4000) ((FunctionSymbol *)symbol)->flags52 |= 0x20;
    }
}

/* Original [0004efc0,0004efcf). Transient eight-byte output dependency. */
BackendSymbolList *new_symbol_list_node(void)
{
    return arena_alloc(4, sizeof(BackendSymbolList));
}

/* Original [0004ed70,0004ee1f). Check referenced initializer symbols
   recursively. Output bit4 is both the recursion mark and the retained
   dependency mark; roll it back when a nested reference is unavailable. */
int __fastcall initializer_symbols_available(SymbolHeader *symbol)
{
    Initializer *initializer;
    SymbolHeader *referenced;
    BackendSymbol *storage;
    if (symbol->kind != 1 || !(symbol->field0e & 0x480)) return 1;
    for (initializer = find_initializer(symbol->identifier)->next; initializer;
         initializer = initializer->next) {
        if (initializer->kind != 2) continue;
        referenced = lookup_global_symbol(initializer->content.relocation.symbol_id);
        if (!referenced) return 0;
        storage = (BackendSymbol *)referenced;
        if ((referenced->kind == 4 || referenced->kind == 1) &&
            (storage->attributes & 0x1000))
            referenced = lookup_alias_target(referenced);
        storage = (BackendSymbol *)referenced;
        if (storage->output_flags & 4) continue;
        storage->output_flags |= 4;
        if (!initializer_symbols_available(referenced)) {
            storage->output_flags &= ~4;
            return 0;
        }
        queue_symbol_debug(referenced);
        if (referenced->kind == 4 && (storage->attributes & 0x4000) &&
            !(storage->attributes & 0x1000))
            ((FunctionSymbol *)referenced)->flags52 |= 0x20;
    }
    return 1;
}
