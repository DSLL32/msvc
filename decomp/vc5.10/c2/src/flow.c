#include "flow.h"
#include "code-input.h"
#include <recon.h>

int flow_block_count;

void __fastcall initialize_flow_pool(C2FlowGraph *flow, int pool,
                                     unsigned bytes, unsigned count)
{
    flow->pool = pool;
    flow->block_bytes = bytes;
    flow->edge_bytes = count;
    flow->first = NULL;
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

C2BasicBlock *__fastcall insert_flow_block(C2FlowGraph *flow, C2BasicBlock *after)
{
    C2BasicBlock *block, *next;
    if (!after)
        return prepend_flow_block(flow);
    block = allocate_flow_block(flow);
    next = after->next;
    block->previous = after;
    block->next = next;
    if (next) {
        after->next = block;
        next->previous = block;
    } else {
        flow->last = block;
        after->next = block;
    }
    return block;
}

C2BasicBlock *__fastcall prepend_flow_block(C2FlowGraph *flow)
{
    C2BasicBlock *block = allocate_flow_block(flow);
    C2BasicBlock *first = flow->first;
    block->next = first;
    if (flow->last) {
        flow->first = block;
        first->previous = block;
    } else {
        flow->last = block;
        flow->first = block;
    }
    return block;
}

C2BasicBlock *__fastcall allocate_flow_block(C2FlowGraph *flow)
{
    C2BasicBlock *block = flow->free_blocks;
    if (!block)
        return c2_pool_allocate(flow->pool, flow->block_bytes);
    flow->free_blocks = block->previous;
    memset(block, 0, flow->block_bytes);
    return block;
}

void __fastcall rebuild_function_flow(C2Function *function)
{
    C2Record *record, *next, *first, *last;
    first = function->flow->last->records;
    last = function->flow->first->last_record;
    for (record = first->next; record != last; record = next) {
        next = record->next;
        if (record->kind == 24) {
            unlink_input_record(record);
            release_small_node(record);
        }
    }
    first = function->flow->last->records;
    last = function->flow->first->last_record;
    reset_input_pool(12);
    create_function_flow(function, first, last, 12);
}

C2BasicBlock *__fastcall split_function_block(C2Function *function,
    C2BasicBlock *block, C2Record *first)
{
    C2Record *boundary = new_statement_boundary();
    C2BasicBlock *result;
    prepend_input_record(first, boundary);
    result = create_function_block(function, block->previous,
                                   boundary, block->last_record);
    result->number86 = block->number86;
    result->scope_depth = block->scope_depth;
    if (first->kind == 25) first->owner.block = result;
    return result;
}

void __fastcall prepend_input_record(C2Record *before, C2Record *record)
{
    C2Record *previous = before->previous;
    before->previous = record;
    record->next = before;
    record->previous = previous;
    previous->next = record;
}

void __fastcall move_input_record_before(C2Record *before, C2Record *record)
{
    unlink_input_record(record);
    prepend_input_record(before, record);
}

void __fastcall unlink_input_record(C2Record *record)
{
    C2Record *previous = record->previous;
    C2Record *next = record->next;
    if (previous) previous->next = next;
    if (next) next->previous = previous;
    record->previous = NULL;
    record->next = NULL;
}
