#include "inline-calls.h"
#include "tree-rewrite.h"
#include "tree-input.h"
#include "records.h"
/* Original [00065030,000650c0). Keep the current inlining directive while
   collecting calls from the function's expression records. */
InlineSite *__fastcall collect_inline_sites(Function *function, int *count)
{
    InlineSite *sites = 0;
    Expression *expression = function->expressions;
    Node *node;
    unsigned flags = 0;
    *count = 0;
    while (expression->tree->opcode != 0x47) {
        node = expression->tree;
        if (node->opcode == 0x4f) {
            if (node->detail.control.code == 0x20) flags = node->left.value;
        } else if (node->opcode != 1 && node->opcode != 0x29 && node->opcode != 0x3a &&
                   node->opcode != 0x46 && node->opcode != 0x56)
            scan_inline_candidates(&sites, expression, node, 0, flags, count);
        expression = expression->next;
    }
    return sites;
}

/* Original [00065520,00065690). Calls are inserted in increasing callee
   cost order. Certain aggregate intrinsics must keep their original form. */
void __fastcall scan_inline_candidates(InlineSite **sites, Expression *expression,
    Node *node, Node *parent, unsigned flags, int *count)
{
    Node *callee, *argument;
    FunctionSymbol *symbol;
    InlineSite *site;
    if (node->opcode == 0x40 && node->left.node->opcode == 0x33 && node->left.node->left.value == 0xdf)
        return;
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        scan_inline_candidates(sites, expression, node->left.node, node, flags, count);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            scan_inline_candidates(sites, expression, node->second.node, node, flags, count);
    }
    if (node->opcode != 0x3e) return;
    callee = node->left.node;
    if (callee->opcode != 0x26 &&
        !(callee->opcode == 0x30 && callee->left.node->opcode == 0x26 &&
          (callee->left.node->right.function->field24 & 0x1000))) return;
    symbol = callee->right.function;
    if ((symbol->symbol.field0e & 0x1000) || !(symbol->field24 & 0x4000) ||
        !function_retains_symbols(symbol, optimize_global)) return;
    if (!(flags & 0xf00) && (node->left.node->right.function->flags52 & 0x10)) return;
    argument = node->second.node;
    while (argument->opcode != 0x4c) {
        if ((argument->attributes.words.type & 0xff00) == 0x8000 && argument->left.node->opcode == 0x40 &&
            argument->left.node->left.node->left.value == 0xdf) return;
        argument = argument->second.node;
    }
    site = new_inline_site(node, parent, expression, flags);
    while (*sites && (short)(*sites)->call->left.node->right.function->field50 <=
                     (short)node->left.node->right.function->field50)
        sites = &(*sites)->next;
    site->next = *sites;
    *sites = site;
    ++*count;
}
