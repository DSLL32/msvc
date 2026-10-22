/* Pending symbol and static-data output. */
#include "symbol-output.h"
/* Original [00068370,00068390). */
void __fastcall emit_pending_symbols(BackendSymbolList *symbols)
{
    flush_queued_symbols();
    write_symbol_declarations();
    emit_static_initializers(symbols);
    write_initializer_listing(symbols);
}

#include "output.h"
BackendSymbolList *queued_output_symbols; /* 0007b93c */
/* Original [00069300,00069330). Callbacks may change the pending list. */
void flush_queued_symbols(void)
{
    while (queued_output_symbols) {
        output_callbacks.mark_symbol(queued_output_symbols->symbol);
        queued_output_symbols = queued_output_symbols->next;
    }
}

#include "options.h"
/* Original [00069330,00069360). */
void __fastcall emit_static_initializers(BackendSymbolList *symbols)
{
    if (!object_filename) return;
    for (; symbols; symbols = symbols->next)
        emit_initializer(find_initializer(symbols->symbol->identifier));
}

/* Original [000692c0,00069300). Emitted symbols need no further queue
   entry; other occurrences are retained for the output callback. */
void __fastcall queue_symbol_debug(SymbolHeader *symbol)
{
    BackendSymbol *storage = (BackendSymbol *)symbol;
    BackendSymbolList *entry;
    if (!(storage->output_flags & 1)) {
        storage->output_flags |= 4;
        entry = arena_alloc(1, sizeof(BackendSymbolList));
        entry->symbol = symbol;
        entry->next = queued_output_symbols;
        queued_output_symbols = entry;
    }
}
