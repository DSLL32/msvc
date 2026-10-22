#ifndef FRAME_SYMBOLS_H
#define FRAME_SYMBOLS_H
#include "physical-allocation.h"
/* Constructor35680 allocates16-byte entries in arena3. Equal-size
   groups order their referenced symbols by descending weighted use count. */
typedef struct FrameSymbolReference {
    struct FrameSymbolReference *next;
    long bytes;
    int weight;
    SymbolHeader *symbol;
} FrameSymbolReference;
typedef char FrameSymbolReferenceSizeCheck[sizeof(FrameSymbolReference) == 16 ? 1 : -1];
extern FrameSymbolReference **frame_symbol_references; /*7c354: current local head*/
void __fastcall clear_local_frame_assignment(SymbolHeader *symbol);
void __fastcall count_frame_symbol_reference(SymbolHeader *symbol, int weight);
#endif
