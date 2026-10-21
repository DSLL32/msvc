#ifndef INITIALIZERS_H
#define INITIALIZERS_H
#include "c2.h"
typedef struct Initializer {
    int kind;
    struct Initializer *next, *link;
    long value;
    unsigned symbol_id;
    void *data;
    unsigned field18;
} Initializer;
extern Initializer *global_initializers;
void __fastcall read_initializers(FILE *file);
Initializer *__fastcall new_initializer(int kind);
void __fastcall read_initializer_body(Initializer *initializer);
void __fastcall insert_initializer(Initializer *initializer);
void __fastcall emit_initializer(Initializer *initializer);
void __fastcall release_initializer(Initializer *initializer);
#endif
