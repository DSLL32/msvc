/* Shared expression trees retained by the global optimizer. */
#include "optimizer-cache.h"
#include "tree-rewrite.h"
OptimizerTreeEntry *optimizer_tree_buckets[64]; /* 0007bc48..0007bd48 */
OptimizerTreeEntry *optimizer_trees; /* 0007bc40 */
/* Original [0005d3f0,0005d430). */
void clear_optimizer_trees(void)
{
    unsigned i;
    OptimizerTreeEntry *entry;
    for (i = 0; i < 64; ++i) {
        for (entry = optimizer_tree_buckets[i]; entry; entry = entry->hash_next)
            release_tree(entry->tree);
        optimizer_tree_buckets[i] = NULL;
    }
    optimizer_trees = NULL;
}
