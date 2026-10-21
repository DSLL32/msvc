/* Static initializer records from the frontend. */
#include "initializers.h"
#include "input.h"
Initializer *global_initializers; /* 0007c14c */
/* Original [0004e830,0004e840). */
void initialize_globals(void)
{
    read_initializers(initializer_file);
}

/* Original [0004e840,0004e8e0). */
void __fastcall read_initializers(FILE *file)
{
    int tag;
    Initializer *initializer;
    input_file = file;
    for (;;) {
        tag = (signed char)input_byte();
        switch (tag) {
        case -1: return;
        case 0:
            initializer = new_initializer(tag);
            initializer->symbol_id = input_varuint();
            initializer->value = input_compact_signed();
            read_initializer_body(initializer);
            insert_initializer(initializer);
            break;
        case 9:
            input_varuint();
            break;
        case 10:
            initializer = new_initializer(tag);
            initializer->symbol_id = 0;
            initializer->value = input_compact_signed();
            read_initializer_body(initializer);
            emit_initializer(initializer);
            release_initializer(initializer);
            break;
        default:
            compiler_internal_error("@(#)p2symtab.c:1.89", 0x5d2);
            break;
        }
    }
}
