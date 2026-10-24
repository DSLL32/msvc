#include "metadata.h"
int metadata_walk(const MetadataRecord *items, unsigned count, MetadataVisit visit)
{
    long total = metadata_sink(&metadata_seed, META_SCALE, 0.5);
    unsigned i, j;
    for (i = 0; i < count; ++i) {
        for (j = 0; j < META_SCALE; ++j) {
            if (items[i].flags & 1) total += visit(&items[i], j);
            else total -= (long)(items[i].weight * (double)(j + 1));
        }
    }
    return (int)(total ^ metadata_mutable ^ META_NAME_LENGTH);
}
