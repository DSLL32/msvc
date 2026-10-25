#include "flow.h"
#include <recon.h>

int flow_block_count;

void __fastcall initialize_flow_pool(C2FlowGraph *flow, int pool,
                                     unsigned bytes, unsigned count)
{
    flow->pool = pool;
    flow->block_bytes = bytes;
    flow->edge_bytes = count;
    flow->entry = NULL;
    flow->last = NULL;
    flow->state08 = 0;
    flow->state0c = 0;
    flow->free_blocks = NULL;
    flow->free_edges = NULL;
}

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

C2BasicBlock *__fastcall create_function_block(C2Function *function,
    C2BasicBlock *after, C2Record *first, C2Record *last)
{
    C2BasicBlock *block = insert_flow_block(function->flow, after);
    block->last_record = last;
    block->records = first;
    block->number84 = 0xffff;
    block->number86 = 0xffff;
    block->number88 = 0xffff;
    first->owner.block = block;
    if (block->next)
        block->next->last_record = first;
    return block;
}
