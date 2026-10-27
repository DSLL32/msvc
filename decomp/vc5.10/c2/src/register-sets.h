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
unsigned __fastcall optimizer_set_contains(C2RegisterSet *set, unsigned number);
struct C2RegisterSet {
    union {
        C2RegisterSetNode *first;
        C2RegisterSet *next_free;
        unsigned *words;
    } links;
    unsigned char pool;
    unsigned char pending05[3];
    unsigned state; /* Low24 bits reset; the high byte is retained. */
};
typedef char C2RegisterSetSizeCheck[sizeof(C2RegisterSet) == 12 ? 1 : -1];
typedef char C2RegisterSetPoolCheck[offsetof(C2RegisterSet,pool) == 4 ? 1 : -1];
typedef char C2RegisterSetStateCheck[offsetof(C2RegisterSet,state) == 8 ? 1 : -1];
/* Header slots0/1/8 and sparse-node slot0 have proved NULL defaults; slots1/3/4/5/6/8/12/13/14 are written by reset.
 * Slots2/7/9..11 are UNMODELED storage, not an original
 * default or object extent. Sparse-node slot8 now has its proved initialized NULL head/tail and ordinary typed queue writers.
 * No base escape, whole-array size/serialization or unknown-index access. */
extern C2RegisterSet *free_register_sets[15];
/* Pool1/3/4/5/6/8/12/13/14 tails are first encountered as a reset writer, not a prior NULL default. */
extern C2RegisterSet *last_free_register_sets[15];
extern C2RegisterSetNode *free_register_set_nodes[15];
extern C2RegisterSetNode *last_free_register_set_nodes[15];
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
extern int register_set_pool_three_initialized;
extern int register_set_pool_four_initialized;
extern int register_set_pool_six_initialized;
extern int register_set_pool_twelve_initialized;
extern int register_set_pool_thirteen_initialized;
extern int register_set_pool_fourteen_initialized;
#define C2_REGISTER_SET_POOL(pool) ((pool) == 0 ? 0 : (pool) == 1 ? 1 : (pool) == 8 ? 8 : ((pool) == 6 && register_set_pool_six_initialized) ? 6 : \
    ((pool) == 3 && register_set_pool_three_initialized) ? 3 : \
    ((pool) == 4 && register_set_pool_four_initialized) ? 4 : \
    ((pool) == 12 && register_set_pool_twelve_initialized) ? 12 : \
    ((pool) == 13 && register_set_pool_thirteen_initialized) ? 13 : \
    ((pool) == 14 && register_set_pool_fourteen_initialized) ? 14 : \
    (recon_unsupported("unmodeled C2 register-set free-head slot"), 1))
#define C2_REGISTER_SET_HEADER_POOL(pool) ((pool) == 8 ? 8 : ((pool) == 4 && register_set_pool_four_initialized) ? 4 : ((pool) == 12 && register_set_pool_twelve_initialized) ? 12 : C2_REGISTER_SET_POOL(pool))
#else
#define C2_REGISTER_SET_POOL(pool) (pool)
#define C2_REGISTER_SET_HEADER_POOL(pool) (pool)
#endif
#endif
