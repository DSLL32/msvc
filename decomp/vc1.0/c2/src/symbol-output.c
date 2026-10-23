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
#include "records.h"
#include <recon.h>
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

SymbolHeader *__fastcall lookup_alias_target(SymbolHeader *symbol);
Segment *initializer_reference_segment; /* original00079150 */
/* Original [00069360,00069415). Emit one symbol's initializer chain with
   temporary segment offsets. Kind10 is inline CodeView type data and
   intentionally retains the updated type-stream position. */
void __fastcall emit_initializer(Initializer *initializer)
{
    SymbolHeader *symbol = lookup_global_symbol(initializer->content.reference.symbol_id);
    Segment *segment;
    unsigned saved_offset;
    if (symbol && (symbol->kind == 1 || symbol->kind == 4) &&
        (((BackendSymbol *)symbol)->attributes & 0x1000)) symbol = lookup_alias_target(symbol);
    if (initializer->kind == 0) segment = (Segment *)lookup_global_symbol(symbol->field18.identifier);
    else if (initializer->kind == 10) segment = debug_types_segment;
    else {
        compiler_internal_error("@(#)emit.c:1.20", 0x1d1);
        return;
    }
    saved_offset = segment->symbol.field04;
    initializer_reference_segment = 0;
    for (; initializer; initializer = initializer->next) {
        if (initializer->kind == 0) segment->symbol.field04 = symbol->field04 + initializer->content.reference.value;
        else if (initializer->kind != 10)
            segment->symbol.field04 += output_callbacks.relocation(initializer, symbol, segment);
    }
    if (segment != debug_types_segment) segment->symbol.field04 = saved_offset;
}
