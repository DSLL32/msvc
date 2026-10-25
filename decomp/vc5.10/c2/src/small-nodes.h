#ifndef C2_SMALL_NODES_H
#define C2_SMALL_NODES_H
#include "function-reader.h"
#include <recon.h>

/* Common prefix only. Node-specific payload sizes come from the kind table. */
struct C2SmallNode {
    C2SmallNode *next;
    unsigned opcode;
    unsigned char kind;
};
typedef char C2SmallNodeKindCheck[offsetof(C2SmallNode, kind) == 8 ? 1 : -1];
#endif
