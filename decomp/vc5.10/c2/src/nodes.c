#include "c2.h"
#include <recon.h>

/* All node variants share the link and kind; their remaining fields differ. */
typedef struct C2NodeHeader {
    struct C2NodeHeader *next;
    signed char kind;
} C2NodeHeader;

unsigned long c2_node_bytes[4];

unsigned __fastcall c2_node_size(int kind)
{
    switch (kind) {
    case 1: case 3: case 11:
        return 64;
    case 2: case 7:
        return 68;
    case 4: case 9:
        return 86;
    case 5: case 8:
        return 70;
    case 14:
        return 110;
    case 16:
        return 90;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 1514);
        return 0; /* The internal diagnostic terminates the process. */
    }
}

void *__fastcall c2_allocate_node(signed char kind, int pool)
{
    unsigned bytes = c2_node_size(kind);
    C2NodeHeader *node;

    c2_node_bytes[pool] += bytes;
    node = c2_pool_allocate(pool, bytes);
    node->kind = kind;
    return node;
}
