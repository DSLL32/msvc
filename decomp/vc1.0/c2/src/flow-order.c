/* Physical ordering of work blocks and explicit fall-through bridges. */
#include "flow-edges.h"
#include "expression-fields.h"
#include "function-input.h"
#include "instruction-selection.h"
#include <recon.h>
static WorkItem *flow_order_head; /* 0007bec0: reverse postorder chain head */

/* Original [0005a540,0005a5d0). Order reachable blocks by the successor
   traversal, then keep the original exit block last in the physical chain. */
void __fastcall order_flow_blocks(WorkItem *head)
{
    WorkItem *work, *last = head, *exit_block, *previous;
    flow_order_head = 0;
    for (work = head; work; work = work->next) {
        work->analysis.traversal_mark = 0;
        global_exit_work = work;
    }
    visit_flow_order(head);
    for (work = head; work; work = work->next) {
        work->analysis.traversal_mark = 0;
        last = work;
    }
    head->previous = 0;
    exit_block = global_exit_work;
    if (last != exit_block) {
        previous = exit_block->previous;
        if (previous && previous->next == exit_block) {
            ensure_flow_branch(previous, exit_block);
            exit_block = global_exit_work;
            exit_block->previous->next = exit_block->next;
            exit_block->next->previous = exit_block->previous;
        } else exit_block->analysis.traversal_mark = 0;
        last->next = exit_block;
        exit_block->previous = last;
        exit_block->next = 0;
    }
}

/* Original [0005b300,0005b370). Prepend a block after recursively visiting
   successors. Preserve displaced physical fall-through with an explicit
   branch block when the selected order changes the original next block. */
void __fastcall visit_flow_order(WorkItem *work)
{
    FlowLink *link;
    WorkItem *following;
    work->analysis.traversal_mark = 0xffff;
    sort_flow_successors(&work->successors, work->next);
    for (link = work->successors; link; link = link->next)
        if (!link->work->analysis.traversal_mark) visit_flow_order(link->work);
    following = work->next;
    if (flow_order_head != following) {
        work->next = flow_order_head;
        if (flow_order_head) flow_order_head->previous = work;
        ensure_flow_branch(work, following);
    } else if (flow_order_head) flow_order_head->previous = work;
    flow_order_head = work;
}

/* Original [0005b370,0005b450). Stable insertion by predecessor count;
   move an exit reference ahead of the ordered links and process a detached
   physical fall-through reference after the remaining candidates. */
void __fastcall sort_flow_successors(FlowLink **links, WorkItem *following)
{
    FlowLink *pending, *link, *exit_link = 0, *fallthrough = 0;
    FlowLink **scan, **position;
    if (!*links) return;
    pending = (*links)->next;
    (*links)->next = 0;
    scan = &pending;
    while (*scan) {
        link = *scan;
        if (link->work == following) {
            fallthrough = link;
            *scan = link->next;
            link->next = 0;
        } else if (link->work == global_exit_work) {
            exit_link = link;
            *scan = link->next;
        } else scan = &link->next;
    }
    if (fallthrough) *scan = fallthrough;
    while (pending) {
        position = links;
        while (*position && count_ordinary_flow_links((*position)->work->predecessors) >=
                            count_ordinary_flow_links(pending->work->predecessors))
            position = &(*position)->next;
        link = pending;
        pending = link->next;
        link->next = *position;
        *position = link;
    }
    if (exit_link) {
        exit_link->next = *links;
        *links = exit_link;
    }
}

/* Original [0005b210,0005b300). Replace a displaced fall-through edge
   with a fresh single-branch block, creating the target's label if needed. */
int __fastcall ensure_flow_branch(WorkItem *source, WorkItem *target)
{
    Expression *last, *expression;
    Node *node;
    LabelSymbol *label;
    WorkItem *branch;
    if (!target) return 0;
    last = last_flow_expression(source->body.expressions);
    if (last && (last->tree->opcode == 0x3a || last->tree->opcode == 0x3c)) return 0;
    expression = target->body.expressions;
    if (expression && expression->tree->opcode == 0x29) {
        label = (LabelSymbol *)expression->tree->right.symbol;
        ++label->references;
    } else {
        node = new_flow_label_node();
        label = (LabelSymbol *)node->right.symbol;
        expression = new_expression();
        expression->tree = node;
        expression->next = target->body.expressions;
        target->body.expressions = expression;
    }
    node = new_node(0x3a);
    node->right.symbol = &label->symbol;
    branch = build_work_lists("SNRnlTL", source, node);
    branch->region = target->region;
    remove_flow_reference(&source->successors, target);
    remove_flow_reference(&target->predecessors, source);
    connect_flow_blocks(source, branch);
    connect_flow_blocks(branch, target);
    branch->flags |= target->flags & source->flags & 1;
    return 1;
}
