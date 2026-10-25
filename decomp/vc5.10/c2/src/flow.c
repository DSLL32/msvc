#include "flow.h"
#include <recon.h>

int flow_block_count;

C2FlowGraph *__fastcall new_flow(int pool)
{
    C2FlowGraph *flow = c2_pool_allocate(pool, sizeof(C2FlowGraph));
    initialize_flow_pool(flow, pool, 140, 20);
    return flow;
}

void __fastcall create_function_flow(C2Function *function, C2Record *first,
                                     C2Record *last, int pool)
{
    flow_block_count = 0;
    function->flow = new_flow(pool);
    create_function_block(function, NULL, first, last);
}
