#ifndef LISTING_H
#define LISTING_H
#include "segments.h"
#include "initializers.h"
#include "machine.h"
#include <stddef.h>
#include <recon.h>
extern unsigned listing_column;
Segment *__fastcall write_segment_listing(Segment *segment, int definition);
Segment *__fastcall close_segment_listing(Segment *segment);
char *__fastcall listing_segment_name(Segment *segment);
typedef void (__fastcall *SymbolVisitor)(SymbolHeader *symbol);
void __fastcall for_each_symbol(int local, SymbolVisitor visit);
void __fastcall write_public_listing(SymbolHeader *symbol);
void __fastcall write_external_listing(SymbolHeader *symbol);
void __fastcall write_common_listing(SymbolHeader *symbol);
void __fastcall mark_listing_symbols(SymbolHeader *symbol);
void __fastcall write_initializer_assembly(Initializer *initializer);
typedef struct ListingSourceLine {
    long line;
    struct ListingSourceLine *next;
} ListingSourceLine;
typedef struct ListingSource {
    char *name;
    FILE *file;
    ListingSourceLine *lines;
    long line;
} ListingSource;
/* Original7c954..7caa4. There are twenty cache slots. The original writes a
   twenty-first entry before diagnosing overflow, aliasing these four globals.
   The indexed view preserves that order without an out-of-bounds C access. */
typedef union ListingStorage {
    struct {
        ListingSource files[20];
        ListingSourceLine *cursor, *lines;
        int enabled;
        Segment *segment;
    } state;
    ListingSource indexed[21];
} ListingStorage;
typedef char ListingSourceSizeCheck[sizeof(ListingSource) == 16 ? 1 : -1];
typedef char ListingStorageSizeCheck[sizeof(ListingStorage) == 336 ? 1 : -1];
/* VC1 does not accept this offsetof macro as an array-bound constant. The
   native checks and original/canonical operand proof bind the field offsets. */
#ifdef RECON_NATIVE_TEST
typedef char ListingCursorOffsetCheck[offsetof(ListingStorage, state.cursor) == 320 ? 1 : -1];
typedef char ListingHeadOffsetCheck[offsetof(ListingStorage, state.lines) == 324 ? 1 : -1];
typedef char ListingEnabledOffsetCheck[offsetof(ListingStorage, state.enabled) == 328 ? 1 : -1];
typedef char ListingSegmentOffsetCheck[offsetof(ListingStorage, state.segment) == 332 ? 1 : -1];
#endif
extern ListingStorage listing_storage;
#define listing_sources (listing_storage.indexed)
#define listing_enabled (listing_storage.state.enabled)
#define listed_segment (listing_storage.state.segment)
extern int listing_source_count, listing_source_index;
extern char *assembly_source_filename;
void __fastcall prepare_source_listing(MachineInstruction *instructions);
void __fastcall open_listing_source(char *filename);
void __fastcall record_listing_source_line(long line);
long __fastcall previous_listing_source_line(long line);
int __fastcall read_listing_source(char *buffer, long line);
int __fastcall write_demangled_listing(SymbolHeader *symbol, int first);
void __fastcall pad_listing_column(unsigned column);
void __fastcall write_listing_symbol(SymbolHeader *symbol);
int __fastcall write_listing_comment(int first);
unsigned __fastcall write_listing_line(unsigned line);
void __fastcall write_listing_source(unsigned line);
MachineInstruction *__fastcall write_instruction_listing(MachineInstruction *instruction, unsigned *offset);
void __fastcall write_listing_bytes(unsigned offset, int count, unsigned char *bytes);
void __fastcall write_listing_spaces(int count);
void __fastcall write_instruction_text(MachineInstruction *instruction);
int __fastcall write_special_instruction(MachineInstruction *instruction);
void __fastcall write_instruction_mnemonic(MachineInstruction *instruction);
void __fastcall write_instruction_operand(MachineInstruction *instruction, MachineOperand *operand, unsigned bytes);
char *__fastcall machine_register_name(unsigned reg, unsigned bytes);
char *__fastcall listing_size_suffix(unsigned bytes);
char *__fastcall listing_data_directive(long bytes);
void __fastcall write_instruction_comment(MachineInstruction *instruction);
void __fastcall write_local_listing(SymbolHeader *symbol);
#endif
