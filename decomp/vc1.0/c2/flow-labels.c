/* Label aliases used while global work blocks are partitioned. */
#include "global-work.h"
typedef struct FlowLabel FlowLabel;
static FlowLabel *flow_label_buckets[32]; /* 0007bed0: label hash heads */
static FlowLabel *pending_flow_labels; /* 00083774: current alias group */

/* Original [0005abc0,0005abd0). End the current run of adjacent labels. */
void reset_flow_label_state(void)
{
    pending_flow_labels = 0;
}

/* Original [0005abb0,0005abc0): a distinct tail adapter in the original. */
void initialize_flow_labels(void)
{
    reset_flow_labels();
}

/* Original [000595f0,00059610). Label descriptors belong to arena four;
   clear their hash heads before returning all of that arena's allocations. */
void reset_flow_labels(void)
{
    unsigned i;
    for (i = 0; i < 32; ++i) flow_label_buckets[i] = 0;
    reset_arena(4);
}
