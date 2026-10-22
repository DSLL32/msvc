/* Work blocks and their temporary control-flow analysis. */
#include "global-work.h"
#include "function-input.h"
#include "tree-rewrite.h"
WorkItem *global_exit_work;       /* 0007ba80: block containing opcode 47 */
WorkItem *global_entry_work;      /* 0007beb8: block containing opcode 46 */
FlowRegion *global_flow_regions;  /* 0007beb0: nested region list */
FlowLink *free_flow_links;        /* 0007bf54: recycled 12-byte edge links */
int global_work_capacity;        /* 0007bebc: maximum numbered-block count */
int global_flow_changed;         /* 0007bf50: region-reduction change flag */

/* Original [000594a0,000595f0). Partition expression records, resolve
   targets, then alternate block numbering and region/edge normalization. */
WorkItem *__fastcall build_global_work(Expression *expressions)
{
    WorkItem *work;
    reset_flow_label_state();
    global_exit_work = 0;
    global_entry_work = 0;
    current_work = 0;
    if (!expressions) return 0;
    current_source_location = 0;
    initialize_flow_labels();
    free_flow_links = 0;
    work = partition_global_work(expressions);
    if (!global_exit_work) return work;
    resolve_flow_labels();
    prepare_flow_blocks(work);
    refresh_flow_edges(work);
    global_work_capacity = 0;
    number_work_blocks(work);
    simplify_flow_branches(work);
    merge_flow_blocks(work);
    refresh_flow_edges(work);
    number_work_blocks(work);
    compute_work_dominators(work);
    find_flow_regions(work);
    if (global_flow_regions) {
        while (simplify_flow_regions(global_flow_regions)) {
            number_work_blocks(work);
            refresh_region_entries(work);
        }
        global_flow_changed = 0;
        reduce_flow_regions(global_flow_regions);
        if (global_flow_changed) number_work_blocks(work);
    }
    analyze_flow_regions(global_flow_regions);
    refresh_flow_edges(work);
    finish_flow_regions(global_flow_regions);
    remove_empty_flow_blocks(work);
    number_work_blocks(work);
    compute_work_dominators(work);
    reset_flow_labels();
    return work;
}

/* Original [0005a910,0005abb0), including the 24-entry terminator table.
   Adjacent source/scope records stay with their block; label aliases and
   region boundaries may force a fresh block or a pending expression. */
WorkItem *__fastcall partition_global_work(Expression *expressions)
{
    WorkItem *head, *work, *result;
    Expression *expression, *previous = 0;
    Node *node, *range;
    int protected_depth = 0, special_depth = 0, handler_depth = 0;
    int new_block = 1;
    set_flow_expression_input(expressions);
    head = work = new_work_item();
    while ((expression = next_flow_expression()) != 0) {
        node = expression->tree;
        if (node->opcode == 1 || (node->opcode == 0x4f &&
            node->detail.control.code >= 0xf && node->detail.control.code <= 0x12)) {
            release_tree(node);
            release_expression(expression);
            continue;
        }
        if (node->opcode == 0x29) {
            expression = fold_adjacent_flow_labels(expression);
            node = expression->tree;
            if (node->opcode != 0x29) {
                putback_flow_expression(expression);
                reset_flow_label_state();
                continue;
            }
            new_block = 1;
        } else if (node->opcode == 0x4f) {
            switch (node->detail.control.code) {
            case 0x19: ++protected_depth; new_block = 1; break;
            case 0x26: ++special_depth; new_block = 1; break;
            case 0x28: case 0x2c: ++handler_depth; break;
            }
        }
        if (new_block) {
            work = build_work_lists("SNLR", work, expression);
            if (expression->tree->opcode == 0x29) {
                define_flow_label(expression->tree->right.symbol, work);
                reset_flow_label_state();
            }
            new_block = 0;
        } else previous->next = expression;
        if (protected_depth > 0) work->flags |= 2;
        if (special_depth > 0) work->flags |= 1;
        if (handler_depth > 0) work->flags |= 0x20;
        node = expression->tree;
        switch (node->opcode) {
        case 0x3c:
            reference_flow_label(node->right.symbol, work);
            for (range = node->second.node; range->opcode != 0x4c; range = range->second.node)
                reference_flow_label(range->right.symbol, work);
            new_block = 1;
            break;
        case 0x46:
            global_entry_work = work;
            /* fall through */
        case 0x38: case 0x3a:
            reference_flow_label(node->right.symbol, work);
            new_block = 1;
            break;
        case 0x47:
            global_exit_work = work;
            break;
        case 0x4f:
            switch (node->detail.control.code) {
            case 0x1a: --protected_depth; new_block = 1; break;
            case 0x2b: case 0x2d: --special_depth; --handler_depth; break;
            }
            break;
        }
        previous = expression;
    }
    if (protected_depth || special_depth || handler_depth)
        compiler_internal_error("@(#)flow.c:1.146", 0x270);
    result = head->next;
    free_work(head);
    return result;
}

static Expression *flow_expression_input; /* 00083778 */
/* Original [0005aec0,0005aed0). Select the chain consumed by the partitioner. */
void __fastcall set_flow_expression_input(Expression *expressions)
{
    flow_expression_input = expressions;
}

/* Original [0005aea0,0005aec0). Detach one expression so block assembly
   can reconnect it without retaining the unread portion of the input chain. */
Expression *next_flow_expression(void)
{
    Expression *expression = flow_expression_input;
    if (expression) {
        flow_expression_input = expression->next;
        expression->next = 0;
    }
    return expression;
}
