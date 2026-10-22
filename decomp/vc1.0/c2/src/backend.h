#ifndef BACKEND_H
#define BACKEND_H
#include "c2.h"
#include <recon.h>
typedef struct Node {
    unsigned opcode;
    union { unsigned value; struct { unsigned short type, flags; } words; } attributes;
    union { unsigned value; struct { short identifier; unsigned short flags; } words; } info;
    union { unsigned value; struct Node *node; void *pointer; short word; } left;
    union { unsigned value; struct Node *node; } second;
    union { unsigned value; struct Node *node; struct FunctionSymbol *function; struct SymbolHeader *symbol; struct Expression *expression; void *pointer; } right;
    union { unsigned value; struct SymbolHeader *symbol; struct Node *node; struct { short code; unsigned short flags; } control; } detail;
} Node;
extern Node *null_node, *scratch_node, *sentinel_node;
unsigned __fastcall descriptor_size(int kind);
void *__fastcall allocate_descriptor(signed char kind, unsigned arena);
void *__fastcall configure_arena(unsigned arena, unsigned mode, unsigned block_size);
void *__fastcall arena_alloc(unsigned arena, unsigned bytes);
void *__fastcall arena_calloc(unsigned arena, unsigned bytes);
typedef struct Arena Arena;
void __fastcall reset_arena(unsigned arena);
void __fastcall free_arena_blocks(Arena *arena);
#endif
