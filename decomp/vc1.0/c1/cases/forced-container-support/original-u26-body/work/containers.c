/* Forced declarations, nested macro rescans and typed record traversal. */
static ContainerItem container_records[CONTAINER_COUNT] = {
    { 11, 1, 2, "first", 1.25 },
    { 17, 0, 3, "second", -2.5 },
    { 23, 1, 4, "third", 3.75 },
    { 29, 1, 5, "fourth", 4.0 }
};
static const char container_title[] = CONTAINER_TITLE;
static struct ContainerHeaderStamp container_stamp = { 1, HEADER_READY };
static volatile int container_sink;

static long container_walk(ContainerSummary *summary, unsigned limit)
{
    unsigned i;
    long total = 0;
    for (i = 0; i < limit && i < CONTAINER_LIMIT; ++i) {
        const ContainerItem *item = container_records + i;
        int bias = CONTAINER_SELECT(item->enabled);
        total += CONTAINER_APPLY(summary->callback, item, bias);
        if (item->weight > 2.0) total += (int)item->weight;
        else total -= item->name[0];
        summary->count++;
    }
    summary->sum = total;
    return total;
}

int container_result(ContainerCallback callback, unsigned limit)
{
    ContainerSummary summary;
    summary.count = 0;
    summary.sum = 0;
    summary.callback = callback;
    container_sink = container_title[0] + container_stamp.version;
    return (int)container_walk(&summary, limit) + (int)summary.count;
}

int main(void)
{
    return container_result(container_deep_score, CONTAINER_LIMIT) == container_sink;
}
