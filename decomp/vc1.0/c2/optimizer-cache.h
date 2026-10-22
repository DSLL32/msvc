#ifndef OPTIMIZER_CACHE_H
#define OPTIMIZER_CACHE_H
#include "functions.h"
/* Entries are 24-byte arena-two descriptors; the producer at 0005da77
   inserts each in its hash bucket and in a second traversal chain. */
typedef struct OptimizerTreeEntry {
    struct OptimizerTreeEntry *hash_next, *next;
    Node *tree;
    unsigned index, field10, field14;
} OptimizerTreeEntry;
extern OptimizerTreeEntry *optimizer_tree_buckets[64], *optimizer_trees;
void clear_optimizer_trees(void);
#endif
