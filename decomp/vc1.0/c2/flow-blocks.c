/* Global work-block simplification, numbering and traversal. */
#include "flow-edges.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
#include "bitsets.h"
#include "records.h"
/* Original [00059a30,00059d30). Keep the entry block fixed while deleting
   unused labels, folding fall-through branches, redirecting a conditional
   around an unconditional branch, and coalescing compatible adjacent blocks.
   Every successful edit retries the current position before advancing. */
void __fastcall simplify_flow_blocks(WorkItem *head)
{
    WorkItem *work = head->next, *next, *following, *target;
    Expression *expression, **tail;
    Node *node;
    SymbolHeader *label;
    FlowLink *link, *links;
    while (work) {
        for (;;) {
            next = work->next;
            if (next && next->body.expressions) {
                expression = next->body.expressions;
                node = expression->tree;
                if (node->opcode == 0x29 && !((LabelSymbol *)node->right.symbol)->references) {
                    release_node(node);
                    next->body.expressions = expression->next;
                    release_expression(expression);
                    continue;
                }
                if (work->body.expressions && work->successors->work == next &&
                    !work->successors->next) {
                    tail = &work->body.expressions;
                    while ((*tail)->next) tail = &(*tail)->next;
                    expression = *tail;
                    node = expression->tree;
                    if (node->opcode == 0x3a) {
                        --((LabelSymbol *)node->right.symbol)->references;
                        release_node(node);
                        release_expression(expression);
                        *tail = 0;
                        continue;
                    }
                    if (node->opcode == 0x38) {
                        --((LabelSymbol *)node->right.symbol)->references;
                        expression->tree = node->left.node;
                        release_node(node);
                        cleanup_expressions(tail);
                        continue;
                    }
                }
            }
            if (next && next->body.expressions &&
                next->body.expressions->tree->opcode == 0x3a &&
                (following = next->next) != 0 &&
                following->body.expressions->tree->opcode == 0x29 &&
                has_flow_reference(work->successors, following)) {
                node = last_flow_expression(work->body.expressions)->tree;
                label = next->body.expressions->tree->right.symbol;
                if (node->opcode == 0x38 &&
                    node->right.symbol == following->body.expressions->tree->right.symbol &&
                    node->right.symbol != label) {
                    node->left.node = negate_condition(node->left.node);
                    next->body.expressions->tree->right.symbol = node->right.symbol;
                    node->right.symbol = label;
                    remove_flow_reference(&work->successors, following);
                    target = next->successors->work;
                    remove_flow_reference(&target->predecessors, next);
                    connect_flow_blocks(work, next->successors->work);
                    next->successors->work = following;
                    remove_flow_reference(&following->predecessors, work);
                    add_flow_predecessor(following, next);
                    continue;
                }
            }
            if (!work->body.expressions) {
                for (link = work->predecessors; link; link = link->next)
                    if (!has_flow_reference(link->work->successors, next))
                        connect_flow_blocks(link->work, next);
                transfer_flow_region_work(work, next);
                remove_flow_work(work);
                work = next;
                continue;
            }
            if (!next || (next->body.expressions && next->body.expressions->tree->opcode == 0x29) ||
                work->successors->work != next || work->successors->next ||
                next->predecessors->work != work || next->predecessors->next ||
                ((work->flags | next->flags) & 2) || ((work->flags ^ next->flags) & 1)) break;
            tail = &work->body.expressions;
            while (*tail) tail = &(*tail)->next;
            *tail = next->body.expressions;
            for (link = next->successors; link; link = link->next) {
                remove_flow_reference(&link->work->predecessors, next);
                add_flow_predecessor(link->work, work);
            }
            for (link = next->predecessors; link; link = link->next) {
                remove_flow_reference(&link->work->successors, next);
                add_flow_successor(link->work, work);
            }
            links = work->successors;
            work->successors = next->successors;
            next->successors = links;
            work->next = next->next;
            if (work->next) work->next->previous = work;
            transfer_flow_region_work(next, work);
            destroy_flow_work(next);
        }
        global_exit_work = work;
        work = work->next;
    }
}

/* Original [00059700,00059720). The stored identifier is a short, while
   the traversal counter and maximum-capacity watermark remain full ints. */
void __fastcall number_work_blocks(WorkItem *work)
{
    int number = 0;
    for (; work; work = work->next) work->number = (short)number++;
    if (number > global_work_capacity) global_work_capacity = number;
}

/* Original [0005a5d0,0005a660). Mark from the entry, remove unvisited
   interior blocks, and retain an unreachable exit shell for later passes. */
void __fastcall remove_unreachable_work(WorkItem *head)
{
    WorkItem *previous, *work;
    Expression *expression;
    if (!global_exit_work) return;
    mark_reachable_work(head);
    previous = head;
    while ((work = previous->next) != 0) {
        if (!(work->flags & 0x10)) {
            if (work != global_exit_work) {
                remove_flow_work(work);
                continue;
            }
            for (expression = work->body.expressions; expression; expression = expression->next) {
                if (expression->tree->opcode == 0x29 || expression->tree->opcode == 0x47) continue;
                release_tree(expression->tree);
                expression->tree = new_node(1);
            }
        }
        previous = work;
    }
    for (work = head; work; work = work->next) work->flags &= ~0x10;
}

/* Original [0005b6a0,0005b6d0). Visit each block once through successor
   links, including protected edges; bit 0x10 is cleared by the caller. */
void __fastcall mark_reachable_work(WorkItem *work)
{
    FlowLink *link;
    if (!(work->flags & 0x10)) {
        work->flags |= 0x10;
        for (link = work->successors; link; link = link->next)
            mark_reachable_work(link->work);
    }
}

/* Original [0005b510,0005b6a0). Unlink edges and the block, dispose of
   its analysis sets and trees, and retain region landmarks for repair. */
void __fastcall remove_flow_work(WorkItem *work)
{
    FlowLink *link;
    FlowRegion *region;
    Expression *expression, *next;
    Node *node, *range;
    LabelSymbol *label;
    unsigned i, source_location = 0;
    int report_code = 0, release_work = 1;
    for (link = work->successors; link; link = link->next)
        remove_flow_reference(&link->work->predecessors, work);
    for (link = work->predecessors; link; link = link->next)
        remove_flow_reference(&link->work->successors, work);
    for (i = 0; i < 6; ++i) {
        release_bitset(work->flow_sets[i]);
        work->flow_sets[i] = 0;
    }
    release_bitset(work->analysis.dominators);
    work->previous->next = work->next;
    work->next->previous = work->previous;
    region = work->region;
    if (region && (region->entry == work || region->header == work || region->last == work)) {
        region->flags |= 1;
        release_work = 0;
    }
    for (expression = work->body.expressions; expression; expression = next) {
        node = expression->tree;
        switch (node->opcode) {
        case 0x29:
            label = (LabelSymbol *)node->right.symbol;
            if (label->label_kind & 4) {
                if (!(label->symbol.field0e & 0x1000))
                    compiler_internal_error("@(#)flow.c:1.146", 0x66a);
                forget_exception_label(current_function, &label->symbol);
            }
            break;
        case 0x38: case 0x3a:
            --((LabelSymbol *)node->right.symbol)->references;
            break;
        case 0x3c:
            for (range = node->second.node; range->opcode != 0x4c; range = range->second.node)
                --((LabelSymbol *)range->right.symbol)->references;
            break;
        default:
            report_code = 1;
            if (!source_location) source_location = expression->source_location;
            break;
        }
        release_tree(node);
        next = expression->next;
        release_expression(expression);
    }
    if (report_code) {
        current_source_location = source_location;
        compiler_warning(4, 0x2be);
        current_source_location = 0;
    }
    if (release_work) destroy_flow_work(work);
}

/* Original [0005b6d0,0005b710). The caller owns tree/analysis cleanup;
   this final step recycles both edge lists and then the work descriptor. */
void __fastcall destroy_flow_work(WorkItem *work)
{
    FlowLink *link, *next;
    for (link = work->successors; link; link = next) {
        next = link->next;
        release_flow_link(link);
    }
    for (link = work->predecessors; link; link = next) {
        next = link->next;
        release_flow_link(link);
    }
    free_work(work);
}

/* Original [0005b710,0005b740). Retarget region landmarks when a work
   block is replaced; an enclosing shared last block is updated separately. */
void __fastcall transfer_flow_region_work(WorkItem *source, WorkItem *replacement)
{
    FlowRegion *region = source->region;
    if (!region) return;
    if (region->entry == source) region->entry = replacement;
    if (region->header == source) region->header = replacement;
    if (region->last == source) replace_flow_region_last(region, replacement);
}
