#ifndef INITIALIZERS_H
#define INITIALIZERS_H
#include "c2.h"
#include "extended.h"
typedef union InitializerValue {
    unsigned char byte;
    short word;
    long integer;
    unsigned float_bits;
    DoubleBits double_bits;
    Extended extended;
    unsigned char *bytes;
} InitializerValue;
typedef struct Initializer {
    int kind;
    struct Initializer *next;
    union {
        struct {
            struct Initializer *link;
            long value;
            unsigned symbol_id;
            void *data;
        } reference;
        struct { short type, bytes; InitializerValue value; } datum;
        struct { unsigned symbol_id; long offset; unsigned char flags; } relocation;
        long argument;
        unsigned char byte_argument;
    } content;
    unsigned repeat;
} Initializer;
extern Initializer *global_initializers;
void __fastcall read_initializers(FILE *file);
Initializer *__fastcall new_initializer(int kind);
void __fastcall read_initializer_body(Initializer *initializer);
void __fastcall insert_initializer(Initializer *initializer);
void __fastcall emit_initializer(Initializer *initializer);
void __fastcall release_initializer(Initializer *initializer);
#endif
