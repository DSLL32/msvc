/* Static initializer records from the frontend. */
#include "initializers.h"
#include "input.h"
/* Original [0004e830,0004e840). */
__declspec(naked) void initialize_globals(void)
{
    __asm {
        mov ecx, initializer_file
        jmp read_initializers
    }
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
            adjust_initializer_debug(initializer);
            release_initializer(initializer);
            break;
        default:
            compiler_internal_error("@(#)p2symtab.c:1.89", 0x5d2);
            break;
        }
    }
}
