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

Node *__fastcall numbered_expression_temporary(Node *node, Node **canonical, int intern);
/* Original [0005dfa0,0005e0a7). Replace a selected numbered value by its
   shared temporary, inserting a defining store before the current expression
   when requested and updating block-local memory uses/definitions. */
Node *__fastcall materialize_numbered_tree(Node *node)
{
    Node *canonical, *load, *store, *root, *reference;
    int identifier;
    if (!bitset_contains(work_clobbered_values, node->info.words.identifier)) return node;
    load = copy_tree(numbered_expression_temporary(node, &canonical, 0));
    load->attributes.words.flags |= node->attributes.words.flags & 0x20;
    root = work_expression->tree;
    if (root->opcode == 0x32 && (root->attributes.words.flags & 0x20) &&
        !trees_equal(node, canonical, 1)) root->attributes.words.flags &= ~0x20;
    if (node->attributes.words.flags & 0x400) {
        store = build_tree("PDNTS", load->left.node, node, 0x32, node);
        store->attributes.words.flags |= 0x20;
        reference = load->detail.node;
        store->detail.node = reference;
        insert_tree_expression(work_expression_anchor, work_expression->tree, store);
        current_expression_work->flow_sets[WORK_MEMORY_STORES] =
            bitset_include(current_expression_work->flow_sets[WORK_MEMORY_STORES], reference->info.words.identifier);
    }
    identifier = load->detail.node->info.words.identifier;
    if (!bitset_contains(current_expression_work->flow_sets[WORK_MEMORY_STORES], identifier))
        current_expression_work->flow_sets[WORK_MEMORY_READS] =
            bitset_include(current_expression_work->flow_sets[WORK_MEMORY_READS], identifier);
    release_tree(node);
    return load;
}
