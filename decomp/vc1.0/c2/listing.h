#ifndef LISTING_H
#define LISTING_H
#include "segments.h"
#include "initializers.h"
extern int listing_enabled;
extern unsigned listing_column;
extern Segment *listed_segment;
void __fastcall write_segment_listing(Segment *segment, int definition);
int __fastcall close_segment_listing(Segment *segment);
char *__fastcall listing_segment_name(Segment *segment);
typedef void (__fastcall *SymbolVisitor)(SymbolHeader *symbol);
void __fastcall for_each_symbol(int local, SymbolVisitor visit);
void __fastcall write_public_listing(SymbolHeader *symbol);
void __fastcall write_external_listing(SymbolHeader *symbol);
void __fastcall write_common_listing(SymbolHeader *symbol);
void __fastcall mark_listing_symbols(SymbolHeader *symbol);
void __fastcall write_initializer_assembly(Initializer *initializer);
#endif
