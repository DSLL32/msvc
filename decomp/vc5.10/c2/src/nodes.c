#include "c2.h"
#include <recon.h>

/* All node variants share the link and kind; their remaining fields differ. */
typedef struct C2NodeHeader {
    struct C2NodeHeader *next;
    signed char kind;
} C2NodeHeader;

unsigned long c2_node_bytes[4];

void *__fastcall c2_allocate_node(signed char kind, int pool)
{
    unsigned bytes = c2_node_size(kind);
    C2NodeHeader *node;

    c2_node_bytes[pool] += bytes;
    node = c2_pool_allocate(pool, bytes);
    node->kind = kind;
    return node;
}
