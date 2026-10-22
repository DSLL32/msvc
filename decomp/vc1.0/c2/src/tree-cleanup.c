#include "tree-rewrite.h"
#include "tree-scratch.h"
#include "expression-fields.h"
#include "global-work.h"
/* Original [00058bd0,00058c80), including its record dispatch table. */
Node *__fastcall cleanup_expression_tree(Node *node)
{
    unsigned opcode = node->opcode;
    if ((opcode >= 0x45 && opcode <= 0x49) || opcode == 0x4f || opcode == 0x56 ||
        (opcode >= 0x8d && opcode <= 0x90) || opcode == 0x9d) return node;
    switch (opcode) {
    case 0x29: case 0x2a: case 0x2b: case 0x38: case 0x3a: case 0x3b: case 0x3c: case 0x41:
        return node;
    default:
        return cleanup_unused_tree(node);
    }
}
/* Original [00058b80,00058bd0). The tree cleanup may change the list
   cursor while splitting an expression, so reload it after every callback. */
void __fastcall cleanup_expressions(Expression **expressions)
{
    Expression *expression = *expressions;
    expression_cleanup_link = expressions;
    while (expression) {
        expression->tree = cleanup_expression_tree(expression->tree);
        if (expression->tree->opcode == 1) {
            *expression_cleanup_link = expression->next;
            release_tree(expression->tree);
            release_expression(expression);
        } else expression_cleanup_link = &expression->next;
        expression = *expression_cleanup_link;
    }
}

/* Original [00058c80,00058db0). Preserve effectful/volatile expressions,
   otherwise retain only operand effects and split two surviving operands. */
Node *__fastcall cleanup_unused_tree(Node *node)
{
    Node *left, *right;
    unsigned arity;
    if ((node_shapes[node->opcode] & NODE_SIDE_EFFECT) ||
        (node->attributes.words.flags & 0x800)) return node;
    if (node->opcode == 0x42 && !can_repeat_tree(node->second.node)) return node;
    arity = node_shapes[node->opcode] & NODE_ARITY;
    switch (arity) {
    case NODE_BINARY: case NODE_CHAIN:
        node->left.node = left = cleanup_unused_tree(node->left.node);
        node->second.node = right = cleanup_unused_tree(node->second.node);
        if (left->opcode == 1) {
            release_node(left);
            release_node(node);
            return right;
        }
        if (right->opcode == 1) {
            release_node(right);
            release_node(node);
            return left;
        }
        build_work_lists("snl", *expression_cleanup_link, right);
        release_node(node);
        return left;
    case NODE_UNARY:
        left = cleanup_unused_tree(node->left.node);
        release_node(node);
        return left;
    case NODE_LEAF:
        release_node(node);
        return new_node(1);
    }
    return node;
}

/* Original [000584e0,00058530). Clone the node and recursively clone
   each operand selected by its arity and second-operand property. */
Node *__fastcall copy_tree(Node *node)
{
    Node *copy = clone_node(node);
    if ((node_shapes[node->opcode] & NODE_ARITY) != NODE_LEAF) {
        copy->left.node = copy_tree(node->left.node);
        if (node_shapes[node->opcode] & NODE_HAS_SECOND)
            copy->second.node = copy_tree(node->second.node);
    }
    return copy;
}
