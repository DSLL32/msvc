#include "flow.h"
#include "code-input.h"
#include "inlining.h"
#include "optimizer-sets.h"
#include <recon.h>

int flow_block_count;
void __fastcall unlink_and_release_input_record(C2Record *record);

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

void __fastcall build_function_edges(C2Function *function)
{
    C2BasicBlock *block, *target;
    C2Expression *record, *value, *last;
    C2SymbolInputUse *use;
    int fallthrough = 0;
    for (block = function->flow->last; block; block = block->previous) {
        if (fallthrough) new_function_edge(function, block->next, block);
        record = (C2Expression *)block->records->next;
        if (record->kind == 25) {
            for (use = record->tail.symbol_uses; use; use = use->next) {
                value = use->expression;
                if (value->kind == 28) value = value->left;
                target = value->next->value.pointer;
                new_function_edge(function, target->next, block);
            }
        }
        last = (C2Expression *)block->last_record->previous;
        if (last->kind == 16)
            fallthrough = last->detail.data != NULL || last->opcode == 0x15c;
        else
            fallthrough = last->kind != 18;
    }
}

C2FlowEdge *__fastcall new_function_edge(C2Function *function,
    C2BasicBlock *source, C2BasicBlock *destination)
{
    C2FlowEdge *edge = new_flow_edge(function->flow, source, destination);
    edge->flags = 0;
    return edge;
}

C2FlowEdge *__fastcall new_flow_edge(C2FlowGraph *flow,
    C2BasicBlock *source, C2BasicBlock *destination)
{
    C2FlowEdge *edge = flow->free_edges;
    if (edge)
        flow->free_edges = edge->next_outgoing;
    else
        edge = c2_pool_allocate(flow->pool, flow->edge_bytes);
    edge->source = source;
    edge->next_outgoing = source->outgoing;
    source->outgoing = edge;
    edge->destination = destination;
    edge->next_incoming = destination->incoming;
    destination->incoming = edge;
    return edge;
}

void __fastcall expand_branch_edges(C2Function *function)
{
    C2BasicBlock *block;
    C2Expression *last;
    for (block = function->flow->last; block; block = block->previous) {
        last = (C2Expression *)block->last_record->previous;
        if (last->kind == 18 && (last->branch_flags & 1)) {
            remove_function_edge(block, last->detail.branch->block, function);
            new_function_edge(function, block, last->detail.branch->block);
            rebuild_branch_target_edges(function, block, last->aggregate_value);
        }
    }
}

void __fastcall insert_function_scope_markers(C2Function *function)
{
    C2BasicBlock *block = function->flow->last;
    C2Record *record;
    for (record = block->records->next; record != block->last_record; record = record->next) {
        if (record->kind != 22) continue;
        insert_input_record(record, (C2Record *)new_control_expression(0x17e));
        if (c2_codeview_types)
            insert_input_record(record, (C2Record *)new_control_expression(0x180));
        break;
    }
    block = function->flow->first;
    for (record = block->records->next; record != block->last_record; record = record->next) {
        if (record->kind != 23) continue;
        prepend_input_record(record, (C2Record *)new_control_expression(0x17f));
        if (c2_codeview_types)
            prepend_input_record(record, (C2Record *)new_control_expression(0x181));
        break;
    }
}

extern unsigned diagnostic_position;
void __fastcall mark_reachable_flow_blocks(C2FlowGraph *flow);
void __fastcall remove_function_block(C2Function *function, C2BasicBlock *block);

void __fastcall normalize_flow_boundaries(C2Function *function)
{
    C2BasicBlock *block, *previous;
    C2Expression *record;
    mark_reachable_flow_blocks(function->flow);
    for (block = function->flow->last; block != function->flow->first; block = previous) {
        previous = block->previous;
        if (block->flags & 1) continue;
        for (record = (C2Expression *)block->records->next;
             record != (C2Expression *)block->last_record; record = record->next) {
            if ((record->flags & 1) && record->kind != 16 && record->kind != 19) {
                diagnostic_position = record->location.line + function->source_line_base;
                compiler_warning(4, 0x2be);
                break;
            }
        }
        remove_function_block(function, block);
    }
}

void __fastcall mark_reachable_flow_blocks(C2FlowGraph *flow)
{
    C2BasicBlock *block = flow->last, *next, *parent;
    C2FlowEdge *edge;
    flow->traversal_last = NULL;
    for (next = block; next; next = next->previous) next->flags &= ~1U;
    block->traversal_edge = block->outgoing;
    block->traversal_next = NULL;
    block->flags |= 1;
    for (;;) {
        edge = block->traversal_edge;
        while (edge && (edge->destination->flags & 1)) edge = edge->next_outgoing;
        if (edge) {
            next = edge->destination;
            block->traversal_edge = edge->next_outgoing;
            next->traversal_edge = next->outgoing;
            next->traversal_next = block;
            next->flags |= 1;
            block = next;
        } else {
            parent = block->traversal_next;
            if (flow->traversal_last) {
                flow->traversal_last->traversal_next = block;
                block->traversal_previous = flow->traversal_last;
            } else {
                flow->traversal_first = block;
                block->traversal_previous = NULL;
            }
            flow->traversal_last = block;
            block = parent;
            if (!block) return;
        }
    }
}

void __fastcall relink_function_block(C2BasicBlock *block, C2BasicBlock *previous, C2Function *function);

void __fastcall order_function_blocks(C2Function *function)
{
    C2FlowGraph *flow = function->flow;
    C2BasicBlock *first = flow->first, *block, *previous, *next;
    C2Record *last = first->last_record, *before;
    flow->last = NULL;
    for (block = flow->traversal_first; block; block = block->traversal_next) {
        previous = flow->last;
        if (!previous) flow->first = block;
        flow->last = block;
        relink_function_block(block, previous, function);
    }
    if (first == flow->first) return;
    next = first->next;
    if (next && next->previous == first) {
        before = first->last_record->previous;
        relink_function_block(next, first->previous, function);
        first->last_record = last;
        before->next = last;
        last->previous = before;
    }
    first->previous = NULL;
    relink_function_block(flow->first, first, function);
    flow->first = first;
}

C2Record *__fastcall find_block_end_record(C2BasicBlock *block)
{
    C2Record *record = block->records->next;
    while (record->next->kind != 24) record = record->next;
    return record;
}
C2InputSymbol *new_flow_label(void);
C2Record *__fastcall prepend_flow_label(C2InputSymbol *symbol, C2BasicBlock *block);
C2Record *__fastcall new_block_jump(C2Record *label, C2Record *prior)
{
    return (C2Record *)new_call_expression(0x10, 0, (C2Expression *)label,
        NULL, NULL, prepend_input_record, prior);
}
int __fastcall repair_relinked_block(C2BasicBlock *block, C2BasicBlock *previous,
    C2Record *last, C2Function *function)
{
    C2Expression *record = (C2Expression *)last;
    C2Record *label, *jump;
    C2BasicBlock *split;
    if (!previous) return 0;
    switch (record->kind) {
    case 16:
        if (!record->detail.branch && record->opcode != 0x15c) return 0;
        break;
    case 17:
        if (record->detail.intrinsic == 0x152) return 0;
        break;
    case 18:
        return 0;
    case 19:
        if (record->opcode == 0x140) return 0;
        break;
    }
    label = previous->records->next;
    if (label->opcode != 0x178)
        label = prepend_flow_label(new_flow_label(), previous);
    jump = new_block_jump(label, block->last_record);
    split = split_function_block(function, block, jump);
    split->block_scope = previous->block_scope;
    remove_function_edge(block, previous, function);
    new_function_edge(function, block, split);
    new_function_edge(function, split, previous);
    return 1;
}
/* The historical name tests whether the second scope is in the first's ancestry:
 * equality of any parent, rather than equality of the two starting pointers. */
int __fastcall block_scopes_equal(C2BlockScope *first, C2BlockScope *second)
{
    for (; first; first = first->parent)
        if (first == second) return 1;
    return 0;
}
void __fastcall repair_block_scope(C2BasicBlock *block, C2BlockScope *scope, C2Function *function);

void __fastcall relink_function_block(C2BasicBlock *block, C2BasicBlock *previous, C2Function *function)
{
    C2BasicBlock *old = block->previous;
    C2Record *last, *boundary;
    if (old == previous) {
        if (previous) previous->next = block;
        return;
    }
    block->previous = previous;
    last = find_block_end_record(block);
    if (previous) {
        boundary = previous->records;
        previous->next = block;
        block->last_record = boundary;
        last->next = boundary;
        boundary->previous = last;
    }
    if (repair_relinked_block(block, old, last, function) && block->block_scope &&
        !block_scopes_equal(old->block_scope, block->block_scope))
        repair_block_scope(block, block->block_scope, function);
}

void __fastcall number_function_blocks(C2Function *function)
{
    C2BasicBlock *block;
    unsigned number = 0;
    for (block = function->flow->last; block; block = block->previous)
        block->number84 = (unsigned short)number++;
    flow_block_count = (short)number;
}

void __fastcall compute_block_dominators(C2Function *function)
{
    C2BasicBlock *first = function->flow->last, *block;
    C2FlowEdge *edge;
    C2RegisterSet *temporary;
    int changed;
    first->dominators = new_optimizer_dense_set(12, flow_block_count);
    add_optimizer_dense_member(first->dominators, 0);
    for (block = first->previous; block; block = block->previous) {
        block->dominators = new_optimizer_dense_set(12, flow_block_count);
        fill_optimizer_dense_set(block->dominators, flow_block_count);
    }
    temporary = new_optimizer_dense_set(12, flow_block_count);
    do {
        changed = 0;
        for (block = first->previous; block; block = block->previous) {
            fill_optimizer_dense_set(temporary, flow_block_count);
            for (edge = block->incoming; edge; edge = edge->next_incoming)
                intersect_optimizer_dense_sets(temporary, edge->source->dominators);
            add_optimizer_dense_member(temporary, (short)block->number84);
            if (!optimizer_dense_sets_equal(temporary, block->dominators)) {
                copy_optimizer_dense_set(block->dominators, temporary);
                changed = 1;
            }
        }
    } while (changed);
    release_optimizer_set_header(temporary);
}

void __fastcall compute_block_predecessors(C2Function *function);
void __fastcall build_flow_regions(C2Function *function, int mode);
void __fastcall release_block_predecessors(C2Function *function);
unsigned __fastcall simplify_flow_regions(C2FlowRegion *regions, C2Function *function);
C2BasicBlock *__fastcall normalize_flow_regions(C2FlowRegion *regions, C2Function *function);
void __fastcall optimize_flow_regions(C2FlowRegion *regions, C2Function *function);
void __fastcall simplify_function_blocks(C2Function *function);
void __fastcall mark_single_flow_region(C2FlowRegion *region, C2Function *function, int depth)
{
    C2BasicBlock *block;
    C2FlowEdge *edge;
    C2FlowBlockLink *link;
    if (!region) return;
    for (block = region->header; block != region->last->previous; block = block->previous) {
        if (block->block_scope == region) block->region_depth = (unsigned short)depth;
        for (edge = block->outgoing; edge; edge = edge->next_outgoing) {
            if (!block_scopes_equal(edge->destination->block_scope, region)) {
                link = c2_pool_allocate(12, sizeof(C2FlowBlockLink));
                link->block = block;
                link->next = region->exits;
                region->exits = link;
                break;
            }
        }
    }
}
void __fastcall mark_flow_regions(C2FlowRegion *regions, C2Function *function, int mode)
{
    for (; regions; regions = regions->next) {
        mark_flow_regions(regions->children, function, mode + 1);
        mark_single_flow_region(regions, function, mode);
    }
}
void __fastcall update_single_flow_region(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *header, *after, *split;
    if (!region || (region->flags & 1)) return;
    header = region->header;
    after = header->next;
    if (after->previous != header)
        c2_report_internal("E:\\utc\\src\\\\P2\\lg.c", 0x342);
    if (after->records->next == after->last_record &&
        after->next->block_scope == region->parent) {
        region->entry = after->next;
        region->rewrite_entry = after;
        after->flags |= 4;
        return;
    }
    split = split_function_block(function, after, header->records);
    split->block_scope = region->parent;
    remove_function_edge(after, header, function);
    new_function_edge(function, after, split);
    new_function_edge(function, split, header);
    region->rewrite_entry = split;
    region->entry = after;
    split->flags |= 4;
}
void __fastcall update_flow_regions(C2FlowRegion *regions, C2Function *function)
{
    for (; regions; regions = regions->next) {
        update_flow_regions(regions->children, function);
        update_single_flow_region(regions, function);
    }
}

int flow_regions_changed; /* 48fa60: this pass resets it before region edits. */
void __fastcall analyze_function_regions(C2Function *function, int mode)
{
    C2BasicBlock *block;
    compute_block_predecessors(function);
    build_flow_regions(function, mode);
    release_block_predecessors(function);
    for (block = function->flow->last; block; block = block->previous) block->flags &= ~4U;
    if (simplify_flow_regions(function->flow_regions, function)) number_function_blocks(function);
    flow_regions_changed = 0;
    if (function->flow_regions) normalize_flow_regions(function->flow_regions, function);
    if (flow_regions_changed) number_function_blocks(function);
    if (mode) optimize_flow_regions(function->flow_regions, function);
    simplify_function_blocks(function);
    if (function->flow_regions) {
        mark_flow_regions(function->flow_regions, function, 1);
        update_flow_regions(function->flow_regions, function);
        if (mode) normalize_flow_boundaries(function);
    }
    number_function_blocks(function);
    compute_block_dominators(function);
}

void __fastcall compute_block_predecessors(C2Function *function)
{
    C2BasicBlock *first = function->flow->last, *block;
    C2FlowEdge *edge;
    C2RegisterSet *temporary, *set;
    int changed;
    for (block = first; block; block = block->previous) {
        block->predecessors = new_optimizer_dense_set(4, flow_block_count);
        add_optimizer_dense_member(block->predecessors, (short)block->number84);
    }
    temporary = new_optimizer_dense_set(4, flow_block_count);
    do {
        changed = 0;
        for (block = first; block; block = block->previous) {
            set = block->predecessors;
            copy_optimizer_dense_set(temporary, set);
            for (edge = block->incoming; edge; edge = edge->next_incoming)
                merge_optimizer_dense_sets(temporary, edge->source->predecessors);
            if (!optimizer_dense_sets_equal(temporary, set)) {
                copy_optimizer_dense_set(set, temporary);
                changed = 1;
            }
        }
    } while (changed);
}

C2FlowRegion *new_flow_region(void);
int __fastcall optimizer_dense_contains(C2RegisterSet *set, unsigned number);

C2InputSymbol *new_flow_label(void);
C2Expression *__fastcall find_control_target(C2Expression *record, C2Expression *target)
{
    C2Symbol *symbol;
    C2Expression *entry;
    if (record->kind == 16) {
        symbol = record->left->kind == 4 ? record->left->value.symbol : NULL;
        return symbol == (C2Symbol *)target->declaration ? record : NULL;
    }
    if (record->kind != 18) return NULL;
    if ((C2Expression *)record->detail.branch == target) return record;
    for (entry = record->aggregate_value; entry; entry = entry->next)
        if (entry->value.branch_record == target) return entry;
    return NULL;
}
void __fastcall move_flow_edge_destination(C2FlowEdge *edge,
    C2BasicBlock *destination, C2Function *function);
void __fastcall redirect_allocation_branch(C2Expression *branch, C2Record *label, int mode);
int __fastcall redirect_flow_target(C2FlowEdge *edge, C2Expression *target,
    C2Record *first, C2Function *function)
{
    C2Expression *branch = find_control_target(
        (C2Expression *)edge->source->last_record->previous, (C2Expression *)first);
    if (!branch) return 0;
    move_flow_edge_destination(edge, ((C2Record *)target)->owner.block, function);
    redirect_allocation_branch(branch, (C2Record *)target, 0);
    return 1;
}
void __fastcall move_flow_edge_destination(C2FlowEdge *edge, C2BasicBlock *destination, C2Function *function);
void __fastcall mark_flow_region(C2FlowRegion *region);
void __fastcall assign_flow_region(C2FlowRegion *region, C2BasicBlock *last)
{
    C2FlowEdge *edge;
    if (last->block_scope == region || last == region->header->next) return;
    last->block_scope = region;
    for (edge = last->incoming; edge; edge = edge->next_incoming)
        assign_flow_region(region, edge->source);
}

void __fastcall build_flow_regions(C2Function *function, int mode)
{
    C2FlowRegion *tail, *region, *parent, **link;
    C2BasicBlock *block, *last, *source, *preheader, *member;
    C2FlowEdge *edge, *backedge, *next;
    C2Expression *record, *label;
    C2Record *first;
    C2InputSymbol *symbol;
    short number;
    int insert = mode;
    tail = new_flow_region();
    function->flow_regions = tail;
    for (block = function->flow->last; block; block = block->previous) {
        last = block;
        for (edge = block->incoming; edge; edge = edge->next_incoming) {
            source = edge->source;
            number = (short)block->number84;
            if (optimizer_dense_contains(source->predecessors, number) &&
                !optimizer_dense_contains(source->dominators, number) &&
                (short)last->number84 < (short)source->number84) last = source;
        }
        if (last != block) {
            region = new_flow_region();
            region->entry = block;
            region->header = block;
            region->flags |= 1;
            region->last = last;
            preheader = block;
        } else {
            number = (short)block->number84;
            for (backedge = block->incoming; backedge; backedge = backedge->next_incoming)
                if (optimizer_dense_contains(backedge->source->dominators, number) &&
                    number <= (short)backedge->source->number84) break;
            if (!backedge) continue;
            if (!insert) {
                preheader = block->next;
                if (preheader->block_scope != block->block_scope) insert = 1;
                else {
                    first = block->records->next;
                    for (edge = block->incoming; edge; edge = edge->next_incoming) {
                        source = edge->source;
                        record = (C2Expression *)source->last_record->previous;
                        if (((1U << (record->kind & 31)) & 0x50000U) && first->kind == 25 &&
                            find_control_target(record, (C2Expression *)first) &&
                            !optimizer_dense_contains(source->dominators, number)) {
                            insert = 1;
                            break;
                        }
                    }
                }
            }
            if (insert) {
                symbol = new_flow_label();
                label = symbol_input_expression(symbol);
                first = block->records->next;
                prepend_input_record(block->records, (C2Record *)label);
                preheader = split_function_block(function, block->next, (C2Record *)label);
                for (edge = block->incoming; edge; edge = next) {
                    next = edge->next_incoming;
                    source = edge->source;
                    number = (short)block->number84;
                    if ((!optimizer_dense_contains(source->dominators, number) ||
                         (short)source->number84 < number) &&
                        !redirect_flow_target(edge, label, first, function))
                        move_flow_edge_destination(edge, preheader, function);
                }
                new_function_edge(function, preheader, block);
            }
            region = new_flow_region();
            region->last = backedge->source;
            region->header = block;
            for (edge = block->incoming; edge; edge = edge->next_incoming)
                if ((short)region->last->number84 < (short)edge->source->number84)
                    region->last = edge->source;
        }
        parent = block->block_scope;
        if (!parent) {
            tail->next = region;
            tail = region;
        } else {
            for (link = &parent->children; *link; link = &(*link)->next) ;
            *link = region;
            region->parent = parent;
            if (insert) preheader->block_scope = parent;
        }
        if ((function->symbol->additional_flags & 0x200) ||
            (function->symbol->symbol.flags & 0x1000)) mark_flow_region(region);
        if (!(region->flags & 1)) assign_flow_region(region, region->last);
        else {
            for (parent = region->parent; parent; parent = parent->parent) parent->flags |= 1;
            for (member = block; member != region->last; member = member->previous)
                member->block_scope = region;
            member->block_scope = region;
        }
    }
    function->flow_regions = function->flow_regions->next;
}

C2FlowRegion *new_flow_region(void)
{
    /* The original allocation is 64 bytes; only its encountered 40-byte prefix
     * is currently named. The remaining bytes acquire types at their users. */
    return c2_pool_allocate(12, 64);
}

void __fastcall release_block_predecessors(C2Function *function)
{
    C2BasicBlock *block;
    for (block = function->flow->last; block; block = block->previous) block->predecessors = NULL;
    reset_input_pool(4);
}

int __fastcall previous_numbered_block(C2BasicBlock *block);
unsigned __fastcall fold_flow_region(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *cursor, *start, *end, *insertion, *last;
    short number, candidate;
    unsigned changed = 0;
    if (!region || (region->flags & 8)) return 0;
    last = region->last;
    for (cursor = region->header; cursor != last; cursor = cursor->previous) {
        if (block_scopes_equal(cursor->previous->block_scope, region)) continue;
        start = cursor->previous;
        end = start;
        while (!block_scopes_equal(end->previous->block_scope, region))
            end = end->previous;
        relink_function_block(cursor, end->previous, function);
        for (insertion = last->previous; insertion; insertion = insertion->previous) {
            if (block_scopes_equal(insertion->block_scope, region)) continue;
            number = (short)start->number84;
            candidate = (short)insertion->number84;
            if (number < candidate ||
                (candidate < 0 && number < previous_numbered_block(insertion))) break;
        }
        if (!insertion) insertion = last->previous;
        relink_function_block(insertion->next, start, function);
        relink_function_block(end, insertion, function);
        changed = 1;
    }
    return changed;
}

unsigned __fastcall simplify_flow_regions(C2FlowRegion *regions, C2Function *function)
{
    unsigned changed = 0, children, result;
    for (; regions; regions = regions->next) {
        children = simplify_flow_regions(regions->children, function);
        result = fold_flow_region(regions, function);
        changed |= children | result;
    }
    return changed;
}

void __fastcall optimize_single_flow_region(C2FlowRegion *region, C2Function *function);

void __fastcall optimize_flow_regions(C2FlowRegion *regions, C2Function *function)
{
    for (; regions; regions = regions->next) {
        optimize_flow_regions(regions->children, function);
        optimize_single_flow_region(regions, function);
    }
}

int __fastcall remove_redundant_flow_branch(C2BasicBlock *block);
int __fastcall simplify_flow_branch(C2BasicBlock *block, C2Function *function);
int __fastcall fold_flow_labels(C2BasicBlock *block, C2Function *function);
C2Expression *__fastcall next_final_marker(C2Expression *record);
void __fastcall rewrite_block_label_use(C2BasicBlock *block, C2Function *function,
    C2Expression *assignment, C2Expression *branch, C2Expression *comparison,
    C2Expression *conditional);
int __fastcall simplify_block_label(C2BasicBlock *block, C2Function *function)
{
    C2Expression *label = (C2Expression *)block->records->next;
    C2Expression *comparison = label->next, *conditional, *marker;
    C2Expression *branch, *assignment, *prior, *prior_branch;
    C2BasicBlock *destination;
    C2SymbolInputUse *use, *next;
    if (comparison->opcode != 0x151 || comparison->left->next->kind != 7) return 0;
    conditional = comparison->next;
    if (conditional->kind != 16) return 0;
    destination = ((C2Record *)((C2InputSymbol *)conditional->left->value.symbol)->
        input_operand->previous)->owner.block;
    if ((short)destination->number84 <= (short)block->number84) return 0;
    marker = next_final_marker(conditional);
    if (marker && marker->kind == 16 && marker->previous->kind == 24) {
        destination = ((C2Record *)((C2InputSymbol *)marker->left->value.symbol)->
            input_operand->previous)->owner.block;
        if ((short)destination->number84 <= (short)block->number84) return 0;
    }
    use = label->tail.symbol_uses;
    if (!use) return 0;
    for (; use; use = use->next) {
        branch = use->expression;
        assignment = branch->previous;
        if (branch->kind != 16) return 0;
        destination = ((C2Record *)branch->next)->owner.block;
        if ((short)destination->number84 >= (short)block->number84) return 0;
        if (assignment->opcode != 0x131 || assignment->left->kind != 7 ||
            compare_input_operands(assignment->tail.right, comparison->left)) return 0;
    }
    assignment = label->previous->previous;
    if (assignment->opcode != 0x131 || assignment->left->kind != 7 ||
        compare_input_operands(assignment->tail.right, comparison->left)) return 0;
    prior = assignment->previous;
    if (prior->kind == 25 && prior->tail.symbol_uses) {
        prior_branch = prior->tail.symbol_uses->expression;
        if (prior_branch->kind == 16) {
            prior_branch = prior_branch->previous;
            if (prior_branch->opcode == 0x151 &&
                !compare_input_operands(assignment->tail.right, prior_branch->left)) return 0;
        }
    }
    for (use = label->tail.symbol_uses; use; use = next) {
        next = use->next;
        branch = use->expression;
        rewrite_block_label_use(block, function, branch->previous, branch,
            comparison, conditional);
    }
    return 1;
}
void __fastcall merge_flow_blocks(C2Function *function, C2BasicBlock *block, C2BasicBlock *previous);

void __fastcall simplify_function_blocks(C2Function *function)
{
    C2BasicBlock *block = function->flow->last->previous, *previous;
    C2Expression *record;
    while (block) {
    retry:
        previous = block->previous;
        if (remove_redundant_flow_branch(block)) goto retry;
        if (simplify_flow_branch(block, function)) goto retry;
        if (fold_flow_labels(block, function)) {
            if (block->next) block = block->next;
            goto retry;
        }
        record = (C2Expression *)block->records->next;
        if (record->kind == 25 && simplify_symbol_record(record)) {
            if (block->next) block = block->next;
            goto retry;
        }
        if (previous && previous->block_scope && previous->block_scope->header == previous) {
            block = previous;
            continue;
        }
        record = (C2Expression *)block->records->next;
        if (record->kind == 25 && simplify_block_label(block, function)) goto retry;
        if (block->block_scope && block->block_scope->header == block) {
            block = previous;
            continue;
        }
        if (block->records->next == block->last_record) {
            merge_flow_blocks(function, block, previous);
            if (block->next) block = block->next;
            goto retry;
        }
        if (previous && block != function->flow->last && previous != function->flow->first &&
            previous->records->next->kind != 25 && previous->incoming &&
            previous->incoming->source == block && !block->outgoing->next_outgoing)
            merge_flow_blocks(function, block, previous);
        block = previous;
    }
}

void __fastcall remove_branch_record(C2Expression *record);
void __fastcall remove_switch_record(C2Expression *record);

int __fastcall remove_redundant_flow_branch(C2BasicBlock *block)
{
    C2FlowEdge *edge;
    C2Expression *record;
    if (!block->previous) return 0;
    for (edge = block->outgoing; edge; edge = edge->next_outgoing)
        if (edge->destination != block->previous) return 0;
    record = (C2Expression *)block->last_record->previous;
    if (record->kind == 16 && record->opcode != 0x15b) {
        remove_branch_record(record);
        return 1;
    }
    if (record->kind == 18) {
        remove_switch_record(record);
        return 1;
    }
    return 0;
}

void __fastcall remove_control_record(C2Expression *record);
void __fastcall prepare_branch_removal(C2Expression *record);

void __fastcall remove_branch_record(C2Expression *record)
{
    if (record->detail.data || record->opcode == 0x15c) prepare_branch_removal(record);
    remove_control_record(record);
}

void __fastcall remove_control_record(C2Expression *record)
{
    C2Expression *previous, *next;
    release_small_node_list(record->detail.data);
    previous = record->previous;
    next = record->next;
    record->detail.data = NULL;
    previous->next = next;
    next->previous = previous;
    release_small_node(record);
}

C2BasicBlock *__fastcall rewrite_flow_branch(C2BasicBlock *block, C2Function *function);

int __fastcall simplify_flow_branch(C2BasicBlock *block, C2Function *function)
{
    C2BasicBlock *previous = block->previous;
    C2Expression *record = (C2Expression *)block->last_record->previous, *other;
    C2Expression *target, *other_target;
    if (record->kind != 16 || (!record->detail.data && record->opcode != 0x15c)) return 0;
    target = record->left;
    if (target->kind != 4 || !((C2InputSymbol *)target->value.symbol)->input_operand || !previous)
        return 0;
    other = (C2Expression *)previous->records->next;
    if (other->kind != 16 || other->detail.data || other->opcode == 0x15c) return 0;
    other_target = other->left;
    if (other_target->kind != 4 || !((C2InputSymbol *)other_target->value.symbol)->input_operand)
        return 0;
    if (previous->block_scope && previous == previous->block_scope->last) return 0;
    if (block->outgoing->destination != previous->previous &&
        block->outgoing->next_outgoing->destination != previous->previous) return 0;
    if (record->opcode == 0x15c || other->opcode == 0x15b ||
        other_target->value.symbol == target->value.symbol) return 0;
    rewrite_flow_branch(block, function);
    return 1;
}

void __fastcall merge_flow_labels(C2Expression *first, C2Expression *last,
    C2Expression *boundary, C2Function *function);

int __fastcall fold_flow_labels(C2BasicBlock *block, C2Function *function)
{
    C2BasicBlock *current = block, *last = block, *boundary = block, *previous;
    C2Expression *label, *record;
    int have_boundary = 0;
    while (current) {
        label = (C2Expression *)current->records->next;
        if (label->kind != 25 ||
            (current->block_scope && current->block_scope->header == current)) break;
        previous = current->previous;
        if (previous && previous->block_scope && previous->block_scope->header == previous) break;
        if (symbol_range_boundary(label)) {
            if (have_boundary) break;
            have_boundary = 1;
            boundary = current;
        }
        last = current;
        for (record = label->next; record != (C2Expression *)current->last_record; record = record->next)
            if (record->kind != 21) goto finish;
        current = current->previous;
    }
finish:
    if (last == block) return 0;
    merge_flow_labels((C2Expression *)block->records->next, (C2Expression *)last->records->next,
        (C2Expression *)boundary->records->next, function);
    return 1;
}

void __fastcall release_function_block(C2Function *function, C2BasicBlock *block);
void __fastcall replace_region_last(C2FlowRegion *region, C2BasicBlock *last)
{
    C2BasicBlock *previous = region->last;
    do {
        region->last = last;
        region = region->parent;
    } while (region && region->last == previous);
}

void __fastcall merge_flow_blocks(C2Function *function, C2BasicBlock *block,
                                  C2BasicBlock *previous)
{
    C2FlowEdge *edge;
    C2FlowRegion *region;
    C2Record *record;
    for (edge = block->incoming; edge; edge = edge->next_incoming)
        new_function_edge(function, edge->source, previous);
    region = block->block_scope;
    if (region) {
        if (region->entry == block) region->entry = previous;
        if (region->header == block) region->header = previous;
        if (region->last == block) replace_region_last(region, previous);
    }
    record = block->records->next;
    if (record->kind == 25) record->owner.block = previous;
    release_function_block(function, block);
}

void __fastcall release_flow_edge(C2FlowGraph *flow, C2FlowEdge *edge);
void __fastcall recycle_flow_block(C2FlowGraph *flow, C2BasicBlock *block);

void __fastcall release_function_block(C2Function *function, C2BasicBlock *block)
{
    C2Record *record = block->last_record;
    C2FlowEdge *edge, *next;
    unlink_input_record(record);
    release_small_node(record);
    for (edge = block->outgoing; edge; edge = next) {
        next = edge->next_outgoing;
        release_flow_edge(function->flow, edge);
    }
    for (edge = block->incoming; edge; edge = next) {
        next = edge->next_incoming;
        release_flow_edge(function->flow, edge);
    }
    if (block->previous) {
        block->previous->records = block->records;
        block->records->owner.block = block->previous;
    }
    if (block->dominators) release_optimizer_set_header(block->dominators);
    recycle_flow_block(function->flow, block);
}

void __fastcall release_flow_edge(C2FlowGraph *flow, C2FlowEdge *edge)
{
    C2FlowEdge **link = &edge->source->outgoing;
    while (*link && *link != edge) link = &(*link)->next_outgoing;
    *link = edge->next_outgoing;
    link = &edge->destination->incoming;
    while (*link && *link != edge) link = &(*link)->next_incoming;
    *link = edge->next_incoming;
    edge->next_outgoing = flow->free_edges;
    flow->free_edges = edge;
}

void __fastcall recycle_flow_block(C2FlowGraph *flow, C2BasicBlock *block)
{
    C2BasicBlock *previous = block->previous, *next = block->next;
    if (next) next->previous = previous;
    else flow->last = previous;
    if (previous) previous->next = next;
    else flow->first = next;
    block->previous = flow->free_blocks;
    flow->free_blocks = block;
}

void __fastcall refine_final_block_dominators(C2Function *function)
{
    C2BasicBlock *first = function->flow->last, *block;
    C2FlowEdge *edge;
    C2RegisterSet *temporary;
    int changed;

    temporary = new_optimizer_dense_set(12, flow_block_count);
    do {
        changed = 0;
        for (block = first->previous; block; block = block->previous) {
            fill_optimizer_dense_set(temporary, flow_block_count);
            for (edge = block->incoming; edge; edge = edge->next_incoming)
                intersect_optimizer_dense_sets(temporary, edge->source->dominators);
            add_optimizer_dense_member(temporary, (short)block->number84);
            if (!optimizer_dense_sets_equal(temporary, block->dominators)) {
                copy_optimizer_dense_set(block->dominators, temporary);
                changed = 1;
            }
        }
    } while (changed);
    release_optimizer_set_header(temporary);
}
void __fastcall release_function_flow_edge(C2Function *function, C2FlowEdge *edge)
{
    release_flow_edge(function->flow, edge);
}
void __fastcall remove_duplicate_block_edges(C2Function *function)
{
    C2BasicBlock *block;
    C2FlowEdge *edge, *next, *scan;

    for (block = function->flow->last; block; block = block->previous) {
        for (edge = block->outgoing; edge; edge = next) {
            next = edge->next_outgoing;
            for (scan = next; scan; scan = scan->next_outgoing) {
                if (scan->destination == edge->destination) {
                    release_function_flow_edge(function, edge);
                    break;
                }
            }
        }
    }
}
void __fastcall refresh_final_flow_analysis(C2Function *function)
{
    insert_function_scope_markers(function);
    normalize_flow_boundaries(function);
    number_function_blocks(function);
    compute_block_dominators(function);
    analyze_function_regions(function, 0);
    refine_final_block_dominators(function);
    remove_duplicate_block_edges(function);
}

void __fastcall remove_function_edge(C2BasicBlock *source,
    C2BasicBlock *destination, C2Function *function)
{
    C2FlowEdge *edge;
    for (edge = source->outgoing; edge; edge = edge->next_outgoing) {
        if (edge->destination == destination) {
            release_function_flow_edge(function, edge);
            return;
        }
    }
}

void __fastcall move_flow_edge_destination(C2FlowEdge *edge,
    C2BasicBlock *destination, C2Function *function)
{
    C2BasicBlock *source = edge->source;
    unsigned flags = edge->flags;
    release_function_flow_edge(function, edge);
    edge = new_function_edge(function, source, destination);
    edge->flags = flags;
}

C2FlowEdge *__fastcall find_flow_edge(C2BasicBlock *source, C2BasicBlock *destination)
{
    C2FlowEdge *edge;
    for (edge = source->outgoing; edge; edge = edge->next_outgoing)
        if (edge->destination == destination) return edge;
    return NULL;
}
C2BasicBlock *__fastcall normalize_flow_regions(C2FlowRegion *regions, C2Function *function)
{
    /* The original reads the first region before its NULL test. Its callers
     * supply a nonempty list; preserve that actual precondition/read. */
    C2BasicBlock *furthest = regions->last, *last, *child_last;
    C2Expression *label;
    C2Record *target;
    C2FlowEdge *edge;
    unsigned short number;
    for (; regions; regions = regions->next) {
        last = regions->last;
        if (regions->children) {
            child_last = normalize_flow_regions(regions->children, function);
            if ((short)child_last->number84 > (short)last->number84 || child_last == last) {
                edge = find_flow_edge(last, regions->header);
                number = child_last->number84;
                if (repair_relinked_block(child_last, child_last->previous,
                    child_last->last_record->previous, function)) {
                    child_last = child_last->previous;
                    child_last->block_scope = regions;
                }
                label = symbol_input_expression(new_flow_label());
                prepend_input_record(child_last->previous->records, (C2Record *)label);
                last = split_function_block(function, child_last, (C2Record *)label);
                target = regions->header->records->next;
                redirect_flow_target(edge, label, target, function);
                last->block_scope = regions;
                regions->last = last;
                last->number84 = number;
                new_block_jump(target, last->last_record);
                new_function_edge(function, last, regions->header);
                flow_regions_changed = 1;
            }
        }
        if ((short)furthest->number84 < (short)last->number84) furthest = last;
    }
    return furthest;
}

extern int input_global_optimization_active;
int __fastcall block_has_call_barrier(C2BasicBlock *block)
{
    C2Expression *record;
    for (record = (C2Expression *)block->records;
         record != (C2Expression *)block->last_record; record = record->next) {
        if ((record->kind == 17 && record->detail.intrinsic == 0xde) ||
            record->kind == 20) return 1;
    }
    return 0;
}
void __fastcall move_flow_edge_source(C2FlowEdge *edge, C2BasicBlock *source,
    C2Function *function)
{
    C2BasicBlock *destination = edge->destination;
    unsigned flags = edge->flags;
    release_function_flow_edge(function, edge);
    edge = new_function_edge(function, source, destination);
    edge->flags = flags;
}
C2FunctionRegister *__fastcall copy_defined_register(C2FunctionRegister *reg, int reuse)
{
    C2FunctionRegister *canonical = reg->canonical;
    if (!reuse || !canonical->copied_register)
        canonical->copied_register = new_temporary_register(canonical->type);
    if (!reg->value_bytes) return reg->copied_register;
    return new_address_register(canonical->copied_register, reg->type,
        reg->value_bytes, (long)((unsigned)reg->displacement - (unsigned)canonical->displacement));
}
C2FunctionRegister *__fastcall mapped_defined_register(C2FunctionRegister *reg)
{
    C2FunctionRegister *canonical;
    if (!reg->value_bytes) return reg->copied_register;
    canonical = reg->canonical;
    return new_address_register(canonical->copied_register, reg->type,
        reg->value_bytes, (long)((unsigned)reg->displacement - (unsigned)canonical->displacement));
}
C2Record *__fastcall copy_input_record(C2Record *record, C2Record *prior)
{
    C2Expression *copy = copy_input_expression((C2Expression *)record);
    C2Expression *operand, *branch, *symbol_owner;
    C2SymbolInputUse *use;
    C2FunctionRegister *original, *replacement, *owner;
    C2InputSymbol *symbol;
    if (record->kind == 25) {
        symbol = new_flow_label();
        copy->declaration = symbol;
        symbol->input_operand = copy;
        branch = (C2Expression *)prior;
        while (branch) {
            if (branch->kind == 16) {
                for (use = copy->tail.symbol_uses; use; use = use->next)
                    if (use->expression == branch) goto label_branch;
            }
            branch = branch->previous;
        }
label_branch:
        /* The original assumes a preceding matching branch. A missing match
         * still reaches redirect_allocation_branch with NULL. */
        redirect_allocation_branch(branch, (C2Record *)copy, 0);
        copy->tail.symbol_uses->next = NULL;
    }
    if (!(copy->flags & 1)) return (C2Record *)copy;
    for (operand = copy->tail.right; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 6) || !operand->value.reg->value) continue;
        original = operand->value.reg;
        replacement = copy_defined_register(original, 0);
        operand->value.reg = replacement;
        if (replacement->canonical->variant_next) bind_input_temporary(replacement, copy);
        else replacement->value = copy;
        if (operand->kind == 1) {
            owner = operand->original_register;
            if (owner->kind == 3 && owner->value) {
                operand->original_register = owner == original ? replacement :
                    copy_defined_register(owner, 1);
            } else if (owner->kind != 1)
                c2_report_internal("E:\\utc\\src\\\\P2\\tuple.c", 0x593);
        }
    }
    for (operand = copy->left; operand; operand = operand->next) {
        if (!((1U << (operand->kind & 31)) & 6) || !operand->value.reg->value) continue;
        original = operand->value.reg;
        replacement = mapped_defined_register(original);
        operand->value.reg = replacement;
        if (operand->kind == 1) {
            owner = operand->original_register;
            if (owner->kind == 3 && owner->value) {
                operand->original_register = owner == original ? replacement :
                    mapped_defined_register(owner);
            } else if (owner->kind != 1)
                c2_report_internal("E:\\utc\\src\\\\P2\\tuple.c", 0x5a6);
        }
    }
    if (copy->kind == 16) {
        if (copy->detail.data) {
            copy->detail.data = NULL;
            attach_call_result(copy, copy->left->next->value.reg->value);
        }
        symbol_owner = ((C2InputSymbol *)copy->left->value.symbol)->input_operand;
        if (symbol_owner) bind_symbol_input_use(symbol_owner, copy);
    }
    return (C2Record *)copy;
}
C2Record *__fastcall copy_input_record_list(C2Record *first)
{
    C2Record *head = copy_input_record(first, NULL), *last = head, *copy;
    for (first = first->next; first; first = first->next) {
        copy = copy_input_record(first, last);
        last->next = copy;
        copy->previous = last;
        last = copy;
    }
    return head;
}

/* Rotate a simple loop's header test into its final block, keeping labels,
 * incoming/outgoing edge ownership and the containing-region endpoints. */
void __fastcall optimize_single_flow_region(C2FlowRegion *region, C2Function *function)
{
    C2BasicBlock *header, *last, *after, *split, *exit_block;
    C2Expression *test, *branch, *first, *label, *exit_label;
    C2FlowEdge *edge, *next, *backedge;
    C2Record *record, *copy;
    if (!region || !input_global_optimization_active || (region->flags & 1)) return;
    header = region->header;
    last = region->last;
    after = header->next;
    for (record = after->records->next; record != after->last_record; record = record->next) ;
    test = (C2Expression *)header->last_record->previous;
    if (test->kind != 16 || (!test->detail.branch && test->opcode != 0x15c) ||
        test->opcode == 0x15c) return;
    for (edge = header->outgoing; edge; edge = edge->next_outgoing)
        if (edge->destination == ((C2Record *)((C2InputSymbol *)
            test->left->value.symbol)->input_operand)->owner.block) break;
    /* The original assumes the test's named target has an outgoing edge. */
    if (block_scopes_equal(edge->destination->block_scope, region)) return;
    if (region->parent && edge->destination->block_scope != region->parent) return;
    branch = (C2Expression *)last->last_record->previous;
    if (branch->kind != 16 || branch->detail.branch || branch->opcode == 0x15c) return;
    backedge = find_flow_edge(last, header);
    if (!backedge || block_has_call_barrier(header)) return;

    test->next = NULL;
    first = (C2Expression *)header->records->next;
    link_code_nodes(after->last_record->previous, (C2Record *)first->next);
    first->next = (C2Expression *)header->last_record;
    header->last_record->previous = (C2Record *)first;
    for (edge = header->outgoing; edge; edge = next) {
        next = edge->next_outgoing;
        if (edge->destination != header->previous)
            move_flow_edge_source(edge, after, function);
    }
    label = (C2Expression *)last->records->next;
    if (label->kind != 25 || branch->previous != label) {
        label = symbol_input_expression(new_flow_label());
        prepend_input_record((C2Record *)branch, (C2Record *)label);
        ((C2Record *)label)->owner.block = last;
    }
    if ((C2Record *)label != last->records->next) {
        split = split_function_block(function, last, (C2Record *)label);
        split->block_scope = last->block_scope;
        for (edge = last->outgoing; edge; edge = next) {
            next = edge->next_outgoing;
            move_flow_edge_source(edge, split, function);
        }
        new_function_edge(function, last, split);
        last = split;
    }
    unlink_and_release_input_record((C2Record *)branch);
    test->next = NULL;
    copy = copy_input_record_list(after->records->next->next);
    test->next = (C2Expression *)after->last_record;
    link_code_nodes((C2Record *)label, copy);
    branch = (C2Expression *)last->last_record->previous;
    redirect_allocation_branch(branch, (C2Record *)first, 0);
    if (branch->condition_code > 16) { __asm { int 3 } }
    branch->condition_code = input_inverse_condition[branch->condition_code];
    branch->flags |= 0x80;
    for (edge = header->incoming; edge; edge = next) {
        next = edge->next_incoming;
        if (edge->source != last && edge->source != header->next &&
            !redirect_flow_target(edge, label, (C2Record *)first, function))
            move_flow_edge_destination(edge, last, function);
    }
    replace_region_last(region, last);
    exit_label = ((C2InputSymbol *)test->left->value.symbol)->input_operand;
    record = new_block_jump((C2Record *)exit_label, last->last_record);
    exit_block = split_function_block(function, last, record);
    exit_block->block_scope = region->parent;
    if (region->parent && region->parent->last == last)
        replace_region_last(region->parent, exit_block);
    new_function_edge(function, last, exit_block);
    new_function_edge(function, exit_block, ((C2Record *)exit_label->previous)->owner.block);
}
