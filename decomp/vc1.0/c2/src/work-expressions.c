/* Materialization and rewriting of numbered work expressions. */
#include "work-expressions.h"
#include "expression-fields.h"
#include "tree-rewrite.h"
#include "tree-input.h"
#include <recon.h>
WorkItem *current_expression_work; /* 0007ba78 */
Expression *work_expression_anchor; /* 0007be8c: dummy head permits insertion before the first tree */

/* Original [0005bd40,0005bda0). Attach a sentinel dummy head while each
   block is rewritten so materialized expressions may precede its old head.
   The native driver ignores the root helper's return value. */
void __fastcall rewrite_work_expressions(WorkItem *work)
{
    Expression *anchor, *expression;
    for (; work; work = work->next) {
        current_expression_work = work;
        anchor = new_expression();
        work_expression_anchor = anchor;
        expression = work->body.expressions;
        anchor->next = expression;
        anchor->tree = intern_leaf(0x4c);
        for (; expression; expression = expression->next) {
            work_expression = expression;
            rewrite_numbered_tree(expression->tree);
        }
        work->body.expressions = work_expression_anchor->next;
    }
}

/* Original [0005df30,0005dfa0). One flag requests materialization before
   visiting children; the other requests it after their numbered rewrites. */
Node *__fastcall rewrite_numbered_tree(Node *node)
{
    if ((node_shapes[node->opcode] & NODE_ARITY) == NODE_LEAF) return node;
    if (node->attributes.words.flags & 0x1000) return materialize_numbered_tree(node);
    node->left.node = rewrite_numbered_tree(node->left.node);
    if (node_shapes[node->opcode] & NODE_HAS_SECOND)
        node->second.node = rewrite_numbered_tree(node->second.node);
    if (node->attributes.words.flags & 0x400) node = materialize_numbered_tree(node);
    return node;
}
