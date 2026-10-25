#ifndef C2_REGISTER_SETS_H
#define C2_REGISTER_SETS_H
#include "function-records.h"
#include <recon.h>

typedef struct C2RegisterSetNode C2RegisterSetNode;
struct C2RegisterSetNode {
    unsigned group;
    C2RegisterSetNode *next;
    unsigned bits;
};
typedef char C2RegisterSetNodeSizeCheck[sizeof(C2RegisterSetNode) == 12 ? 1 : -1];
typedef char C2RegisterSetNodeNextCheck[offsetof(C2RegisterSetNode,next) == 4 ? 1 : -1];
typedef char C2RegisterSetNodeBitsCheck[offsetof(C2RegisterSetNode,bits) == 8 ? 1 : -1];
C2RegisterSetNode *__fastcall new_register_set_node(C2RegisterSet *set);
struct C2RegisterSet {
    union {
        C2RegisterSetNode *first;
        C2RegisterSet *next_free;
    } links;
    unsigned char pool;
    unsigned char pending05[3];
    unsigned state; /* Low24 bits reset; the high byte is retained. */
};
typedef char C2RegisterSetSizeCheck[sizeof(C2RegisterSet) == 12 ? 1 : -1];
typedef char C2RegisterSetPoolCheck[offsetof(C2RegisterSet,pool) == 4 ? 1 : -1];
typedef char C2RegisterSetStateCheck[offsetof(C2RegisterSet,state) == 8 ? 1 : -1];
/* Encountered prefix only. Slot0 is UNMODELED storage, not an original
 * default or object extent. Original pool12 is valid but not modeled here.
 * No base escape, whole-array size/serialization or unknown-index access. */
extern C2RegisterSet *free_register_sets[2];
extern C2RegisterSetNode *free_register_set_nodes[2];
extern C2RegisterSetNode *last_free_register_set_nodes[2];
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define C2_REGISTER_SET_POOL(pool) ((pool) == 1 ? 1 : \
    (recon_unsupported("unmodeled C2 register-set free-head slot"), 1))
#else
#define C2_REGISTER_SET_POOL(pool) (pool)
#endif
#endif
