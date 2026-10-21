#ifndef BACKEND_H
#define BACKEND_H
#include "c2.h"
typedef struct Node {
    unsigned opcode;
    unsigned field04, field08, field0c, field10, field14, field18;
} Node;
extern Node *null_node, *scratch_node, *sentinel_node;
void *__fastcall configure_arena(unsigned arena, unsigned mode, unsigned block_size);
void *__fastcall arena_calloc(unsigned arena, unsigned bytes);
#endif
