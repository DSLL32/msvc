/* Canonical work values retained by the global optimizer. */
#include "optimizer-cache.h"
#include "tree-rewrite.h"
#include <recon.h>
/* Original [0005d3f0,0005d430). The hash heads at0007bc48 and reverse
   creation root at0007bc40 are the same globals the value interner updates.
   Release each retained tree before reset_work discards arena two. */
void clear_optimizer_trees(void)
{
    unsigned i;
    WorkValue *entry;
    for (i = 0; i < WORK_VALUE_BUCKETS; ++i) {
        for (entry = work_value_buckets[i]; entry; entry = entry->hash_next)
            release_tree(entry->canonical);
        work_value_buckets[i] = NULL;
    }
    last_work_value = NULL;
}
