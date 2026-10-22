#ifndef LISTING_H
#define LISTING_H
#include "segments.h"
#include "initializers.h"
#include "machine.h"
extern int listing_enabled;
extern unsigned listing_column;
extern Segment *listed_segment;
Segment *__fastcall write_segment_listing(Segment *segment, int definition);
int __fastcall close_segment_listing(Segment *segment);
char *__fastcall listing_segment_name(Segment *segment);
typedef void (__fastcall *SymbolVisitor)(SymbolHeader *symbol);
void __fastcall for_each_symbol(int local, SymbolVisitor visit);
void __fastcall write_public_listing(SymbolHeader *symbol);
void __fastcall write_external_listing(SymbolHeader *symbol);
void __fastcall write_common_listing(SymbolHeader *symbol);
void __fastcall mark_listing_symbols(SymbolHeader *symbol);
void __fastcall write_initializer_assembly(Initializer *initializer);
typedef struct ListingSource {
    char *name;
    FILE *file;
    unsigned line, position;
} ListingSource;
extern ListingSource listing_sources[20];
extern int listing_source_count, listing_source_index;
extern char *assembly_source_filename;
void __fastcall prepare_source_listing(MachineInstruction *instructions);
void __fastcall open_listing_source(char *filename);
int __fastcall write_demangled_listing(SymbolHeader *symbol, int first);
void __fastcall pad_listing_column(unsigned column);
void __fastcall write_listing_symbol(SymbolHeader *symbol);
int __fastcall write_listing_comment(int first);
unsigned __fastcall write_listing_line(unsigned line);
void __fastcall write_listing_source(unsigned line);
MachineInstruction *__fastcall write_instruction_listing(MachineInstruction *instruction, unsigned *offset);
void __fastcall write_local_listing(SymbolHeader *symbol);
#endif
