#ifndef SYMBOL_OUTPUT_H
#define SYMBOL_OUTPUT_H
#include "machine.h"
#include "initializers.h"
#include <recon.h>
extern BackendSymbolList *queued_output_symbols;
void flush_queued_symbols(void);
void write_symbol_declarations(void);
void __fastcall emit_static_initializers(BackendSymbolList *symbols);
void __fastcall write_initializer_listing(BackendSymbolList *symbols);
Initializer *__fastcall find_initializer(unsigned identifier);
void __fastcall emit_initializer(Initializer *initializer);
#endif
