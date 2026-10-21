/* Symbol and scope descriptor allocation. */
#include "backend.h"
typedef struct DescriptorHeader {
    void *next;
    unsigned field04, field08;
    signed char kind;
    unsigned char field0d;
    unsigned short field0e;
} DescriptorHeader;
unsigned descriptor_bytes[7];
unsigned __fastcall descriptor_size(signed char kind);
void *__fastcall arena_alloc(unsigned arena, unsigned bytes);
/* Original [0004dcb0,0004dce0). */
void *__fastcall allocate_descriptor(signed char kind, unsigned arena)
{
    unsigned bytes = descriptor_size(kind);
    DescriptorHeader *descriptor;
    descriptor_bytes[arena] += bytes;
    descriptor = arena_alloc(arena, bytes);
    descriptor->kind = kind;
    return descriptor;
}

/* Original [0004dd90,0004de50), including its .text switch table. */
unsigned __fastcall descriptor_size(signed char kind)
{
    switch (kind) {
    case 1: case 11: return 44;
    case 2: case 7: return 48;
    case 3: case 8: return 50;
    case 4: return 64;
    case 5: return 52;
    case 9: return 72;
    case 14: return 84;
    case 16: return 68;
    case 19: return 13;
    default:
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x47d);
        return 0;
    }
}
