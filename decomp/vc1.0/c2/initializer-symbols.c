/* Resolve dependencies of static initializer records. */
#include "machine.h"
#include "initializers.h"
#include "records.h"
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
    for (initializer = global_initializers; initializer; initializer = initializer->link) {
        symbol = lookup_global_symbol(initializer->symbol_id);
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
    for (initializer = global_initializers; initializer; initializer = initializer->link) {
        symbol = lookup_global_symbol(initializer->symbol_id);
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
