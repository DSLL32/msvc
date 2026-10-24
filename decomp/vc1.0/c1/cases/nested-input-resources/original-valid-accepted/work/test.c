#include "resource1.h"
static ResourceItem resource_items[] = { {1, 7, 15}, {2, -3, 9}, {3, 5, 12} };
int resource_walk(ResourceBatch *batch, ResourceVisit visit, ResourceSummary *summary)
{
    unsigned i;
    long total = 0;
    for (i = 0; i < batch->count; ++i) total += visit(&batch->items[i], RESOURCE_SCALE);
    summary->count = batch->count;
    summary->total = total;
    return total != 0;
}
int main(void)
{
    ResourceBatch batch;
    ResourceSummary summary;
    batch.items = resource_items;
    batch.count = sizeof(resource_items) / sizeof(resource_items[0]);
    return !resource_walk(&batch, resource_value, &summary);
}
