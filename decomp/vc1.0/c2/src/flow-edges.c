/* Predecessor/successor construction for global work blocks. */
#include "flow-edges.h"
#include "tree-rewrite.h"
#include "function-input.h"
#include "expression-fields.h"
#include "instruction-selection.h"
#include "records.h"
#include <recon.h>

/* Original [0005a660,0005a8e0), including the 24-entry control table.
   Materialize predecessor references, normalize branch labels, then add
   ordinary and protected successor edges for each expression block. */
void __fastcall prepare_flow_blocks(WorkItem *work)
{
    WorkItem *fallthrough = 0;
    ExceptionScope *scope = 0, *enclosing;
    FlowLink *reference, *references, *next;
    Expression *expression;
    Node *node, *last, *range;
    for (; work; work = work->next) {
        if (fallthrough) add_flow_predecessor(work, fallthrough);
        node = work->body.expressions->tree;
        if (node->opcode == 0x29) {
            references = lookup_flow_label(node->right.symbol)->references;
            for (reference = references; reference; reference = reference->next)
                add_flow_predecessor(work, reference->work);
            for (reference = references; reference; reference = next) {
                next = reference->next;
                release_flow_link(reference);
            }
        }
        last = normalize_flow_targets(work->body.expressions);
        if (work->flags & 3) {
            for (expression = work->body.expressions; expression; expression = expression->next) {
                node = expression->tree;
                if (node->opcode != 0x4f) continue;
                switch (node->detail.control.code) {
                case 0x16:
                    visit_machine_symbols(node->left.pointer, reference_assembly_flow_symbol, work);
                    break;
                case 0x26:
                    scope = node->left.pointer;
                    scope->state = 0x8f;
                    break;
                case 0x28: case 0x2a: case 0x2c:
                    scope = node->left.pointer;
                    scope->state = 1;
                    break;
                case 0x29:
                    scope = node->left.pointer;
                    add_protected_flow_edge(work, lookup_flow_label(scope->handler_label)->work);
                    break;
                case 0x2d:
                    add_protected_flow_edge(work, global_exit_work);
                    /* fall through */
                case 0x27: case 0x2b:
                    scope = node->left.pointer;
                    break;
                }
            }
            if (work->flags & 1) {
                work->exception_scope = (short)scope->identifier;
                for (enclosing = scope;;) {
                    if (enclosing->state == 0x8f) {
                        add_protected_flow_edge(work, lookup_flow_label(enclosing->handler_label)->work);
                        if (enclosing->kind == 1)
                            add_protected_flow_edge(work, lookup_flow_label(enclosing->filter_label)->work);
                    }
                    if (enclosing->parent_identifier == -1) break;
                    enclosing = lookup_exception_scope(current_function->exception_scopes,
                                                       enclosing->parent_identifier);
                }
            }
        }
        switch (last->opcode) {
        case 0x38:
            if (work->next) add_flow_successor(work, work->next);
            fallthrough = work;
            add_flow_successor(work, lookup_flow_label(last->right.symbol)->work);
            break;
        case 0x3a:
            add_flow_successor(work, lookup_flow_label(last->right.symbol)->work);
            fallthrough = 0;
            break;
        case 0x3c:
            add_flow_successor(work, lookup_flow_label(last->right.symbol)->work);
            for (range = last->second.node; range->opcode != 0x4c; range = range->second.node)
                add_flow_successor(work, lookup_flow_label(range->right.symbol)->work);
            fallthrough = 0;
            break;
        default:
            if (work->next) add_flow_successor(work, work->next);
            fallthrough = work;
            break;
        }
    }
}

/* Original [0005afd0,0005b100). Redirect every branch operand to its
   alias representative and count uses of the resulting label symbols.
   The returned node is the block's final expression tree. */
Node *__fastcall normalize_flow_targets(Expression *expression)
{
    Node *node, *range;
    FlowLabel *label;
    ExceptionScope *scope;
    SymbolHeader *symbol;
    for (;;) {
        node = expression->tree;
        switch (node->opcode) {
        case 0x38: case 0x3a:
            symbol = lookup_flow_label(node->right.symbol)->canonical->label;
            node->right.symbol = symbol;
            ++((LabelSymbol *)symbol)->references;
            break;
        case 0x3c:
            symbol = lookup_flow_label(node->right.symbol)->canonical->label;
            node->right.symbol = symbol;
            ++((LabelSymbol *)symbol)->references;
            for (range = node->second.node; range->opcode != 0x4c; range = range->second.node) {
                symbol = lookup_flow_label(range->right.symbol)->canonical->label;
                range->right.symbol = symbol;
                ++((LabelSymbol *)symbol)->references;
            }
            break;
        case 0x4f:
            switch (node->detail.control.code) {
            case 0x16:
                visit_machine_symbols(node->left.pointer, normalize_assembly_flow_symbol, 0);
                break;
            case 0x26:
                scope = node->left.pointer;
                label = lookup_flow_label(scope->handler_label);
                if (label->canonical != label)
                    compiler_internal_error("@(#)flow.c:1.146", 0x473);
                ++((LabelSymbol *)label->label)->references;
                if (scope->kind == 1) {
                    label = lookup_flow_label(scope->filter_label);
                    if (label->canonical != label)
                        compiler_internal_error("@(#)flow.c:1.146", 0x478);
                } else {
                    label = lookup_flow_label(scope->alternate_label);
                    if (label->canonical != label)
                        compiler_internal_error("@(#)flow.c:1.146", 0x47f);
                }
                ++((LabelSymbol *)label->label)->references;
                break;
            }
            break;
        }
        if (!expression->next) return expression->tree;
        expression = expression->next;
    }
}

/* Original [0005af00,0005af40). Ordinary edges are distinct from protected
   edges to the same block, selected by the link's low flag bit. */
void __fastcall add_flow_successor(WorkItem *work, WorkItem *successor)
{
    FlowLink *link;
    for (link = work->successors; link; link = link->next)
        if (link->work == successor && !(link->flags & 1)) return;
    link = new_flow_link(successor);
    link->next = work->successors;
    work->successors = link;
}

/* Original [0005b110,0005b150). Add one ordinary predecessor, preserving
   a separate protected predecessor even when the block identity agrees. */
void __fastcall add_flow_predecessor(WorkItem *work, WorkItem *predecessor)
{
    FlowLink *link;
    for (link = work->predecessors; link; link = link->next)
        if (link->work == predecessor && !(link->flags & 1)) return;
    link = new_flow_link(predecessor);
    link->next = work->predecessors;
    work->predecessors = link;
}

/* Original [0005b100,0005b110). Only the next pointer changes on release. */
void __fastcall release_flow_link(FlowLink *link)
{
    link->next = free_flow_links;
    free_flow_links = link;
}


/* Original [0005b170,0005b1a0). The requested edge must be present;
   like the original, absence reaches the null link rather than being ignored. */
void __fastcall remove_flow_reference(FlowLink **links, WorkItem *work)
{
    FlowLink *removed, *next;
    while (*links && (*links)->work != work) links = &(*links)->next;
    removed = *links;
    next = removed->next;
    release_flow_link(removed);
    *links = next;
}

/* Original [0005b450,0005b470). Protected edges do not contribute to
   the predecessor count used when ordering physical fall-through. */
int __fastcall count_ordinary_flow_links(FlowLink *links)
{
    int count = 0;
    for (; links; links = links->next) if (!(links->flags & 1)) ++count;
    return count;
}

/* Original [0005b470,0005b490). Return the final record, or zero for an
   empty expression chain. */
Expression *__fastcall last_flow_expression(Expression *expression)
{
    if (expression) while (expression->next) expression = expression->next;
    return expression;
}

/* Original [0005b150,0005b170). Build both directions of an ordinary edge. */
void __fastcall connect_flow_blocks(WorkItem *source, WorkItem *target)
{
    add_flow_successor(source, target);
    add_flow_predecessor(target, source);
}

/* Original [0005b490,0005b504). Redirect the block's terminal branch or
   switch destinations, transferring each reference between label counters. */
void __fastcall redirect_flow_label(WorkItem *work, SymbolHeader *old_label, SymbolHeader *new_label)
{
    Node *node;
    if (!work->body.expressions) return;
    node = last_flow_expression(work->body.expressions)->tree;
    if (node->opcode == 0x38 || node->opcode == 0x3a) {
        if (node->right.symbol == old_label) {
            node->right.symbol = new_label;
            ++((LabelSymbol *)new_label)->references;
            --((LabelSymbol *)old_label)->references;
        }
        return;
    }
    if (node->opcode != 0x3c) return;
    for (;;) {
        if (node->right.symbol == old_label) {
            node->right.symbol = new_label;
            ++((LabelSymbol *)new_label)->references;
            --((LabelSymbol *)old_label)->references;
        }
        node = node->second.node;
        if (node->opcode == 0x4c) break;
    }
}

/* Original [00027920,00027995). Walk explicit operands in reverse count
   order, retaining the actual symbol slot across callback/lookup mutations.
   A callback returns zero to keep the slot, or a global identifier to replace
   it. The signed opcode count and opaque context keep their original roles. */
void __fastcall visit_machine_symbols(MachineInstruction *instruction,
    MachineSymbolVisitor visitor, void *context)
{
    int count = machine_operand_counts[(instruction->opcode >> 8) & 255];
    MachineOperand *operand;
    SymbolHeader **slot;
    unsigned kind, identifier;
    for (; count > 0; --count) {
        operand = &instruction->operands[count == 2 ? 1 : 0];
        kind = operand->flags & 15;
        if (kind >= OPERAND_ABSOLUTE && kind <= OPERAND_LABEL_ADDRESS)
            slot = &operand->value.address->symbol;
        else if (kind == OPERAND_SYMBOL) slot = &operand->value.symbol;
        else slot = NULL;
        if (slot && *slot) {
            identifier = visitor(*slot, context);
            if (identifier) *slot = lookup_global_symbol(identifier);
        }
    }
}
